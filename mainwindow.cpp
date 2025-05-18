#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "equipmentmanagementform.h"
#include "energycalibrationform.h"
#include "spectrumModel.h"
#include "offlinedataanalysiswidget.h"
#include "plotwidget.h"
#include "yieldcalibration.h"
#include "FPGASetting.h"
#include "splashwidget.h"

#include <QFileDialog>
#include <QToolButton>
#include <QTimer>
#include <QScreen>
#include <QButtonGroup>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QWhatsThis>
#include <iostream>
#include <QShortcut>
#include <windows.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    multi_CHANNEL = 8192;

    emit sigRefreshBoostMsg(tr("更新ui..."));
    // 电源
    this->setProperty("relay_on", false);
    this->setProperty("relay_fault", false);
    //外设
    this->setProperty("control_on", false);
    this->setProperty("control_fault", false);
    //探测器
    this->setProperty("detector_fault", false);
    this->setProperty("detector_on", false);
    //测量
    this->setProperty("measure-status", msNone);
    //测量模式
    this->setProperty("measure-model", mmNone);
    //暂停刷新
    this->setProperty("pause_plot", false);
    //线性模型
    this->setProperty("ScaleLogarithmicType", false);

    ui->spinBox_timeWidth->setToolTip("请输入10的倍数（如10, 20, 30）");
    ui->spinBox_timeWidth->setSingleStep(10);
    connect(ui->spinBox_timeWidth, QOverload<int>::of(&QSpinBox::valueChanged), [=](int value) {
        if (value % 10 != 0) {
            ui->spinBox_timeWidth->blockSignals(true);
            ui->spinBox_timeWidth->setValue((value / 10) * 10);
            ui->spinBox_timeWidth->blockSignals(false);
        }
    });

    ui->spinBox_timeWidth_2->setToolTip("请输入10的倍数（如10, 20, 30）");
    ui->spinBox_timeWidth_2->setSingleStep(10);
    connect(ui->spinBox_timeWidth_2, QOverload<int>::of(&QSpinBox::valueChanged), [=](int value) {
        if (value % 10 != 0) {
            ui->spinBox_timeWidth_2->blockSignals(true);
            ui->spinBox_timeWidth_2->setValue((value / 10) * 10);
            ui->spinBox_timeWidth_2->blockSignals(false);
        }
    });

    // 日志窗口增加清空日志功能
    // 1. 启用自定义上下文菜单
    ui->textEdit_log->setContextMenuPolicy(Qt::CustomContextMenu);
    // 2. 连接信号到槽函数
    connect(ui->textEdit_log, &QTextEdit::customContextMenuRequested,
        this, [=](const QPoint &pos){
        QMenu *menu = ui->textEdit_log->createStandardContextMenu();
    
        // 添加分隔线
        menu->addSeparator();

        // 添加"清空"动作
        QAction *clearAction = menu->addAction(tr("清空内容"));
        connect(clearAction, &QAction::triggered, this, [this]() {
            ui->textEdit_log->clear();
        });

        // 显示菜单
        menu->exec(ui->textEdit_log->viewport()->mapToGlobal(pos));

        // 删除菜单对象
        delete menu;
    });

    connect(this, SIGNAL(sigRefreshUi()), this, SLOT(slotRefreshUi()));
    connect(this, SIGNAL(sigAppengMsg(const QString &, QtMsgType)), this, SLOT(slotAppendMsg(const QString &, QtMsgType)));

    commandHelper = CommandHelper::instance();
    controlHelper = ControlHelper::instance();

    connect(controlHelper, &ControlHelper::sigError, this, [=](QString msg){
        // emit sigAppengMsg(msg, QtMsgType::QtWarningMsg);
        qCritical().noquote()<<msg;
    });

    //外设状态
    connect(controlHelper, &ControlHelper::sigControlFault, this, [=](){
        this->setProperty("control_fault", true);
        this->setProperty("control_on", false);

        emit sigRefreshUi();
    });

    connect(controlHelper, &ControlHelper::sigControlStatus, this, [=](bool on){
        this->setProperty("control_fault", false);
        this->setProperty("control_on", on);

        QString msg = QString(tr("外设状态：%2")).arg(on ? tr("开") : tr("关"));
        QLabel* label_Connected = this->findChild<QLabel*>("label_Connected");
        label_Connected->setText(msg);
        // emit sigAppengMsg(msg, QtInfoMsg);
        qInfo().noquote()<<msg;
        emit sigRefreshUi();

        //外设连接成功，主动连接电源
        if (this->property("control_on").toBool()){
            //这里采用异步触发，避免堵塞主界面刷新状态
            QTimer::singleShot(500, this, [=](){
                commandHelper->openRelay(true);

                QPair<float, float> pair = controlHelper->gotoAbs(ui->comboBox_range->currentIndex());
                this->setProperty("axis01-target-position", pair.first);
                this->setProperty("axis02-target-position", pair.second);

                SplashWidget::instance()->setInfo(tr("量程正在设置中，请等待...\n正在移动位移平台至目标位置..."));
                SplashWidget::instance()->exec();
            });
        }
    });

    this->setProperty("axis-prepared", false);//位移平台是否移动到指定量程
    connect(controlHelper, &ControlHelper::sigReportAbs, this, [=](float f1, float f2){
        QLabel* label_axis01 = this->findChild<QLabel*>("label_axis01");
        label_axis01->setText(tr("轴1：") + QString::number(f1 / 1000, 'f', 4) + " mm");

        QLabel* label_axis02 = this->findChild<QLabel*>("label_axis02");
        label_axis02->setText(tr("轴2：") + QString::number(f2 / 1000, 'f', 4) + " mm");

        float axis01_target_position = this->property("axis01-target-position").toFloat();
        float axis02_target_position = this->property("axis02-target-position").toFloat();

        // 误差5um
        if (qAbs(f1/1000-axis01_target_position)<=MAX_MISTAKE_VALUE && qAbs(f2/1000-axis02_target_position)<=MAX_MISTAKE_VALUE){
            if (!this->property("axis-prepared").toBool())
            {
                this->setProperty("axis-prepared", true);
                emit sigRefreshUi();

                SplashWidget::instance()->hide();
                //emit sigAppengMsg(tr("位移平台已到位"), QtInfoMsg);
                qInfo().noquote() << tr("位移平台已到位");
            }
        } else{
            this->setProperty("axis-prepared", false);
        }
    });

    // 禁用开启电源按钮
    ui->action_power->setEnabled(false);
    connect(commandHelper, &CommandHelper::sigRelayFault, this, [=](){
        this->setProperty("relay_fault", true);
        this->setProperty("relay_on", false);

        QString msg = QString(tr("网络故障，电源连接失败！"));
        // emit sigAppengMsg(msg, QtCriticalMsg);
        qCritical().noquote()<<msg;
        emit sigRefreshUi();
    });

    connect(commandHelper, &CommandHelper::sigAppendMsg2, this, [=](const QString & msg, QtMsgType msgType){
        qInfo().noquote()<<msg;
    }, Qt::QueuedConnection);

    connect(commandHelper, &CommandHelper::sigRelayStatus, this, [=](bool on){
        this->setProperty("relay_fault", false);
        this->setProperty("relay_on", on);

        if (!on){
            //电源关闭，设备断电，测量强制停止
            this->setProperty("measure-status", msEnd);
        }else{
            if (!this->property("last_safe_exit").toBool()){
                //如果上次是异常退出，这次先关闭继电器，再开继电器
                commandHelper->closeRelay();

                QTimer::singleShot(500, this, [=](){
                    this->setProperty("last_safe_exit", true);
                    commandHelper->openRelay(true);

                    QPair<float, float> pair = controlHelper->gotoAbs(ui->comboBox_range->currentIndex());
                    this->setProperty("axis01-target-position", pair.first);
                    this->setProperty("axis02-target-position", pair.second);

                    SplashWidget::instance()->setInfo(tr("量程正在设置中，请等待...\n正在移动位移平台至目标位置..."));
                    SplashWidget::instance()->exec();
                });
            } else {
                QPair<float, float> pair = controlHelper->gotoAbs(ui->comboBox_range->currentIndex());
                this->setProperty("axis01-target-position", pair.first);
                this->setProperty("axis02-target-position", pair.second);

                SplashWidget::instance()->setInfo(tr("量程正在设置中，请等待...\n正在移动位移平台至目标位置..."));
                SplashWidget::instance()->exec();
            }
        }

        if (this->property("last_safe_exit").toBool()){
            //异常退出先不要恢复继电器状态，等重启继电器之后再说吧

            QString msg = QString(tr("电源状态：%1")).arg(on ? tr("开") : tr("关"));
            // emit sigAppengMsg(msg, QtInfoMsg);
            qInfo().noquote()<<msg;
            emit sigRefreshUi();
        }
    });

    // 测量开始
    connect(commandHelper, &CommandHelper::sigMeasureStart, this, [=](qint8 mmode, qint8 tmode){
        if (tmode == 0x02 || tmode == 0x03)
            return;//波形测量、能谱测量

        this->lastRecvDataTime = QDateTime::currentDateTime();
        this->setProperty("measure-status", msStart);
        this->setProperty("measure-model", mmode);

        //启动异常检测机制
        QTimer* exceptionCheckTimer = this->findChild<QTimer*>("exceptionCheckTimer");
        exceptionCheckTimer->start(1000);

        if (mmode == mmAuto)
        {
            ui->lineEdit_autoStatus->setText("冷却状态");
                         
            //自动测量的冷却时间倒计时
            QTimer* coolingTimer_auto = this->findChild<QTimer*>("coolingTimer_auto");
            coolingTimer_auto->start(ui->spinBox_coolingTime_2->value()*1000);
        }
            // ui->start_time_text->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
            // qInfo().noquote()<<"接收到触发信号，仪器内部时钟开始计时";//注意，在sigMeasureStart信号之前已经打印了qInfo().noquote()日志

        //开启测量时钟
        if (mmode == mmManual || mmode == mmAuto){//手动/自动测量
            //测量倒计时时钟

            //能谱测量不需要开启倒计时
            if (tmode != 0x00){
                QTimer* measureTimer = this->findChild<QTimer*>("measureTimer");
                measureTimer->start();

                //测量时长时钟
                this->measureStartTime = this->lastRecvDataTime;
                QTimer* measureRefTimer = this->findChild<QTimer*>("measureRefTimer");
                measureRefTimer->start(500);
            }
        }

        QString msg = tr("测量正式开始");
        // emit sigAppengMsg(msg, QtInfoMsg);
        qInfo().noquote()<<msg;
        emit sigRefreshUi();
    });

    connect(commandHelper, &CommandHelper::sigMeasureWait, this, [=](){
        this->setProperty("measure-status", msWaiting);
    });

    // 给出测量结果
    connect(commandHelper, &CommandHelper::sigActiveOmiga, this, [=](double a){
        //本次测量量程
        int measureRange = ui->comboBox_range->currentIndex();
        //读取配置文件，给出该量程下的刻度系数
        double cali_factor = 0.0;
        QFile file(QApplication::applicationDirPath() + "/config/user.json");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // 读取文件内容
            QByteArray jsonData = file.readAll();
            file.close(); //释放资源
    
            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
            QJsonObject jsonObj = jsonDoc.object();
            
            QJsonObject jsonCalibration, jsonYield;
            if (jsonObj.contains("YieldCalibration")){
                jsonCalibration = jsonObj["YieldCalibration"].toObject();
                
                QString key = QString("Range%1").arg(measureRange);
                QJsonArray rangeArray;
                if (jsonCalibration.contains(key)){
                    rangeArray = jsonCalibration[key].toArray();
                    QJsonObject rangeData = rangeArray[0].toObject();

                    double yield = rangeData["Yield"].toDouble();
                    double active0 = rangeData["active0"].toDouble();
                    cali_factor = yield / active0;
                }
            }
        }
        else{
            qCritical().noquote()<<"未找到拟合参数，请检查仪器是否进行了刻度,请对仪器刻度后重新计算（采用‘数据查看和分析’子界面分析）";
        }

        double result = a * cali_factor;
        ui->lineEdit_CuActive->setText(QString::number(a, 'E', 5));
        ui->lineEdit_Yield->setText(QString::number(result, 'E', 5));
    });

    // 测量停止
    connect(commandHelper, &CommandHelper::sigMeasureStop, this, [=](){
        QTimer* measureTimer = this->findChild<QTimer*>("measureTimer");
        measureTimer->stop();

        QTimer* measureRefTimer = this->findChild<QTimer*>("measureRefTimer");
        measureRefTimer->stop();

        QTimer* exceptionCheckTimer = this->findChild<QTimer*>("exceptionCheckTimer");
        exceptionCheckTimer->stop();

        this->setProperty("measure-status", msEnd);
        QString msg = tr("测量已停止");
        // emit sigAppengMsg(msg, QtInfoMsg);
        qInfo().noquote()<<msg;
        emit sigRefreshUi();
    });

    // 禁用连接探测器按钮
    ui->action_detector_connect->setEnabled(false);
    connect(commandHelper, &CommandHelper::sigDetectorFault, this, [=](){
        this->setProperty("detector_fault", true);
        this->setProperty("detector_on", false);
        this->setProperty("measure-status", msEnd);

        QTimer* measureTimer = this->findChild<QTimer*>("measureTimer");
        measureTimer->stop();

        QString msg = QString(tr("网络故障，探测器连接失败！"));
        // emit sigAppengMsg(msg, QtCriticalMsg);
        qCritical().noquote()<<msg;
        emit sigRefreshUi();
    });

    connect(commandHelper, &CommandHelper::sigDetectorStatus, this, [=](bool on){
        this->setProperty("detector_fault", false);
        this->setProperty("detector_on", on);
        if (!on){
            this->setProperty("measure-status", msEnd);
            QTimer* measureTimer = this->findChild<QTimer*>("measureTimer");
            measureTimer->stop();
        }

        QString msg = QString(tr("探测器状态：%1")).arg(on ? tr("开") : tr("关"));
        // emit sigAppengMsg(msg, QtInfoMsg);
        qInfo().noquote()<<msg;
        emit sigRefreshUi();
    });

    qRegisterMetaType<SingleSpectrum>("SingleSpectrum");
    qRegisterMetaType<vector<CoincidenceResult>>("vector<CoincidenceResult>");
    std::cout << "main thread id:" << QThread::currentThreadId() << std::endl;
    connect(commandHelper, &CommandHelper::sigPlot, this, [=](SingleSpectrum r1, vector<CoincidenceResult> r3){
        this->lastRecvDataTime = QDateTime::currentDateTime();
        bool pause_plot = this->property("pause_plot").toBool();
        if (!pause_plot){
            PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
            plotWidget->slotUpdatePlotDatas(r1, r3);

            if(r3.size()==0) return;
            ui->lcdNumber_CountRate1->display(r3.back().CountRate1);
            ui->lcdNumber_CountRate2->display(r3.back().CountRate2);
            ui->lcdNumber_ConCount_single->display(r3.back().ConCount_single);
            ui->lcdNumber_DeathRatio1->display(r3.back().DeathRatio1);
            ui->lcdNumber_DeathRatio2->display(r3.back().DeathRatio2);
        }
    }, Qt::DirectConnection/*防止堵塞*/);
    //DirectConnection replot 子线程操作，不会堵塞，但是会崩溃
    //QueuedConnection replot 主线程操作，刷新慢

    connect(commandHelper, &CommandHelper::sigUpdateAutoEnWidth, this, [=](std::vector<unsigned short> EnWidth, qint8 mmode){
        if (mmode == mmManual )
        {
            ui->spinBox_1_leftE->setValue(EnWidth[0]);
            ui->spinBox_1_rightE->setValue(EnWidth[1]);
    
            ui->spinBox_2_leftE->setValue(EnWidth[2]);
            ui->spinBox_2_rightE->setValue(EnWidth[3]);
        }else if(mmode == mmAuto)
        {
            ui->spinBox_1_leftE_2->setValue(EnWidth[0]);
            ui->spinBox_1_rightE_2->setValue(EnWidth[1]);
    
            ui->spinBox_2_leftE_2->setValue(EnWidth[2]);
            ui->spinBox_2_rightE_2->setValue(EnWidth[3]);
        }

        PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
        plotWidget->slotUpdateEnWindow(EnWidth.data());

        // qDebug().noquote()<<"自动更新能窗，探测器1:["<<EnWidth[0]<<","<<EnWidth[1]
        //                   <<"], 探测器2:["<<EnWidth[2]<<","<<EnWidth[3]<<"]";
    }, Qt::QueuedConnection/*防止堵塞*/);

    emit sigRefreshUi();

    // 创建图表
    emit sigRefreshBoostMsg(tr("创建图形库..."));
    initCustomPlot();

    emit sigRefreshBoostMsg(tr("读取参数设置ui..."));
    InitMainWindowUi();

    QTimer::singleShot(500, this, [=](){
       this->showMaximized();
    });

    commandHelper->startWork();
}

MainWindow::~MainWindow()
{
    delete ui;
}

#include <QMouseEvent>
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != this){
        if (event->type() == QEvent::MouseButtonPress){
            if (watched->inherits("QLabel")){
                QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
                if (e->button() == Qt::LeftButton) {
                    QLabel* label = qobject_cast<QLabel*>(watched);
                    bool toggled = false;
                    if (label->property("toggled").isValid())
                        toggled = label->property("toggled").toBool();

                    toggled = !toggled;
                    label->setProperty("toggled", toggled);
                    if (toggled){
                        label->setPixmap(QPixmap(":/resource/arrow-drop-up-x16.png"));
                        QWidget * w = label->buddy();
                        w->hide();
                    } else {
                        label->setPixmap(QPixmap(":/resource/arrow-drop-down-x16.png"));
                        QWidget * w = label->buddy();
                        w->show();
                    }
                }
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::InitMainWindowUi()
{
#ifdef QT_NO_DEBUG
    ui->toolBar_online->removeAction(ui->action_start_measure);
    ui->menu_4->removeAction(ui->action_Moving); // 屏蔽掉位移平台子界面
    ui->menu_view->removeAction(ui->action_SpectrumModel); //屏蔽掉能谱界面
#endif

    QActionGroup *actGroup = new QActionGroup(this);
    actGroup->setExclusive(true);
    actGroup->addAction(ui->action_move);
    actGroup->addAction(ui->action_drag);
    actGroup->addAction(ui->action_tip);

    ui->toolBar_offline->hide();

    // 获取当前时间
    ui->textEdit_log->clear();

    ui->spinBox_1_leftE->setMaximum(MULTI_CHANNEL);
    ui->spinBox_1_rightE->setMaximum(MULTI_CHANNEL);
    ui->spinBox_2_leftE->setMaximum(MULTI_CHANNEL);
    ui->spinBox_2_rightE->setMaximum(MULTI_CHANNEL);

    ui->spinBox_1_leftE_2->setMaximum(MULTI_CHANNEL);
    ui->spinBox_1_rightE_2->setMaximum(MULTI_CHANNEL);
    ui->spinBox_2_leftE_2->setMaximum(MULTI_CHANNEL);
    ui->spinBox_2_rightE_2->setMaximum(MULTI_CHANNEL);

    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/user.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {

        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        // 将 JSON 数据解析为 QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        QString cachePath = QApplication::applicationDirPath() + "/cache";
        QDir dir(cachePath);
        if (!dir.exists())
            dir.mkdir(cachePath);
        QString cacheDir = QCoreApplication::applicationDirPath() + "/cache";
        if(jsonObj.contains("defaultCache")) cacheDir = jsonObj["defaultCache"].toString();
        commandHelper->setDefaultCacheDir(cacheDir);
        // ui->lineEdit_path->setText(jsonObj["path"].toString());
        // ui->lineEdit_filename->setText(jsonObj["filename"].toString());
    } else {
        // ui->lineEdit_path->setText("./");
        // ui->lineEdit_filename->setText("test.dat");
        commandHelper->setDefaultCacheDir("./cache");
    }

    // 测量结果另存为-存储路径
    /*{
        QAction *action = ui->lineEdit_path->addAction(QIcon(":/resource/open.png"), QLineEdit::TrailingPosition);
        QToolButton* button = qobject_cast<QToolButton*>(action->associatedWidgets().last());
        button->setCursor(QCursor(Qt::PointingHandCursor));
        connect(button, &QToolButton::pressed, this, [=](){
            QString dir = QFileDialog::getExistingDirectory(this);
            ui->lineEdit_path->setText(dir);
        });
    }*/

    ui->tabWidget_measure->setCurrentIndex(0);

    // 启用关闭按钮
    ui->tabWidget_client->setTabsClosable(true);
    ui->tabWidget_client->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);//第一个tab取消关闭按钮
    connect(ui->tabWidget_client, &QTabWidget::tabCloseRequested, this, [=](int index){
        ui->tabWidget_client->removeTab(index);
        if (index == 1){
            this->setProperty("offline-filename", "");
            delete offlineDataAnalysisWidget;
            offlineDataAnalysisWidget = nullptr;
        }
    });

    connect(ui->comboBox_range, QOverload<int>::of(&QComboBox::currentIndexChanged), ui->comboBox_range_2, &QComboBox::setCurrentIndex);
    connect(ui->comboBox_range_2, QOverload<int>::of(&QComboBox::currentIndexChanged), ui->comboBox_range, &QComboBox::setCurrentIndex);
    connect(ui->comboBox_range, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index){
        if (controlHelper->connected()){
            QPair<float, float> pair = controlHelper->gotoAbs(index);
            this->setProperty("axis01-target-position", pair.first);
            this->setProperty("axis02-target-position", pair.second);

            SplashWidget::instance()->setInfo(tr("量程正在设置中，请等待...\n正在移动位移平台至目标位置..."));
            SplashWidget::instance()->exec();
        }
    });

    // 显示计数/能谱
    QButtonGroup *grp = new QButtonGroup(this);
    grp->addButton(ui->radioButton_ref, 0);
    grp->addButton(ui->radioButton_spectrum, 1);
    connect(grp, QOverload<int>::of(&QButtonGroup::idClicked), this, [=](int index){
        commandHelper->switchToCountMode(index == 0);
        PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
        plotWidget->switchToCountMode(index == 0);
    });
    emit grp->idClicked(1);//默认能谱模式

    // 任务栏信息
    QLabel *label_Idle = new QLabel(ui->statusbar);
    label_Idle->setObjectName("label_Idle");
    label_Idle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    label_Idle->setFixedWidth(300);
    label_Idle->setText(tr("就绪"));

    QLabel *label_Connected = new QLabel(ui->statusbar);
    label_Connected->setObjectName("label_Connected");
    label_Connected->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    label_Connected->setFixedWidth(300);
    label_Connected->setText(tr("外设状态：未连接"));

    QLabel *label_axis01 = new QLabel(ui->statusbar);
    label_axis01->setObjectName("label_axis01");
    label_axis01->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    label_axis01->setFixedWidth(300);
    label_axis01->setText(tr("轴1：未就绪"));

    QLabel *label_axis02 = new QLabel(ui->statusbar);
    label_axis02->setObjectName("label_axis02");
    label_axis02->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    label_axis02->setFixedWidth(300);
    label_axis02->setText(tr("轴2：未就绪"));

    /*设置任务栏信息*/
    QLabel *label_systemtime = new QLabel(ui->statusbar);
    label_systemtime->setObjectName("label_systemtime");
    label_systemtime->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ui->statusbar->setContentsMargins(5, 0, 5, 0);
    ui->statusbar->addWidget(label_Idle);
    ui->statusbar->addWidget(label_Connected);
    ui->statusbar->addWidget(label_axis01);
    ui->statusbar->addWidget(label_axis02);
    ui->statusbar->addWidget(new QLabel(ui->statusbar), 1);
    ui->statusbar->addWidget(nullptr, 1);
    ui->statusbar->addPermanentWidget(label_systemtime);

    //日志窗口重置位置
    // int horizontalDPI = logicalDpiX();
    int verticalDPI  = logicalDpiY();
    int screenValidHeight = QGuiApplication::primaryScreen()->size().height();
    screenValidHeight = screenValidHeight *  96 / verticalDPI;
    if (screenValidHeight <= 768){
        // 分页
        QWidget *tabWidget_workLog = new QWidget();
        tabWidget_workLog->setObjectName(QString::fromUtf8("tabWidget_workLog"));
        QHBoxLayout *horizontalLayout_21 = new QHBoxLayout(tabWidget_workLog);
        horizontalLayout_21->setObjectName(QString::fromUtf8("horizontalLayout_21"));
        horizontalLayout_21->setContentsMargins(0, 0, 0, 0);
        QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy3.setHeightForWidth(ui->widget_log->sizePolicy().hasHeightForWidth());
        ui->widget_log->setSizePolicy(sizePolicy3);

        //删除标题栏
        QWidget* lTitleBar = ui->dockWidget_2->titleBarWidget();
        QWidget* lEmptyWidget = new QWidget();
        ui->dockWidget_2->setTitleBarWidget(lEmptyWidget);
        delete lTitleBar;

        horizontalLayout_21->addWidget(ui->widget_log);
        ui->widget_log->setMaximumHeight(16777215);        
        int index = ui->tabWidget_client->addTab(tabWidget_workLog/*ui->widget_log*/, tr("日志视窗"));
        ui->tabWidget_client->tabBar()->setTabButton(index, QTabBar::RightSide, nullptr);//tab取消关闭按钮
    } else if (screenValidHeight <= 1080){
        // 右侧
        ui->widget_right->layout()->removeItem(ui->verticalSpacer_3);//移除弹簧        
        QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->widget_log->setSizePolicy(sizePolicy3);
        ui->widget_log->setMaximumHeight(16777215);
        ui->widget_right->layout()->addWidget(ui->widget_log);
        //ui->widget_right->layout()->addItem(ui->verticalSpacer_3);//移除弹簧
    } else {

    }

    QTimer* systemClockTimer = new QTimer(this);
    systemClockTimer->setObjectName("systemClockTimer");
    connect(systemClockTimer, &QTimer::timeout, this, [=](){
        // 获取当前时间
        QDateTime currentDateTime = QDateTime::currentDateTime();

        // 获取星期几的数字（1代表星期日，7代表星期日）
        int dayOfWeekNumber = currentDateTime.date().dayOfWeek();

        // 星期几的中文名称列表
        QStringList dayNames = {
            tr("星期日"), QObject::tr("星期一"), QObject::tr("星期二"), QObject::tr("星期三"), QObject::tr("星期四"), QObject::tr("星期五"), QObject::tr("星期六"), QObject::tr("星期日")
        };

        // 根据数字获取中文名称
        QString dayOfWeekString = dayNames.at(dayOfWeekNumber);
        this->findChild<QLabel*>("label_systemtime")->setText(QString(QObject::tr("系统时间：")) + currentDateTime.toString("yyyy/MM/dd hh:mm:ss ") + dayOfWeekString);
    });
    systemClockTimer->start(900);

    QTimer* measureTimer = new QTimer(this);
    measureTimer->setObjectName("measureTimer");
    connect(measureTimer, &QTimer::timeout, this, [=](){
        QString msg = tr("定时测量倒计时结束，自动停止测量！");
        // emit sigAppengMsg(msg, QtInfoMsg);
        qInfo().noquote()<<msg;

        //自动测量时间到，停止测量
        measureTimer->stop();
        if (this->property("measure-model").toUInt() == mmManual)
            commandHelper->slotStopManualMeasure();
        else
            commandHelper->slotStopAutoMeasure();

        QMessageBox::information(this, tr("提醒"), tr("定时测量倒计时结束，自动停止测量！"));
    });

    QTimer* measureRefTimer = new QTimer(this);
    measureRefTimer->setObjectName("measureRefTimer");
    connect(measureRefTimer, &QTimer::timeout, this, [=](){
        QDateTime currentDateTime = QDateTime::currentDateTime();
        qint64 days = currentDateTime.daysTo(measureStartTime);
        if (days >= 1){
            ui->lcdNumber->display(QString::number(days) + tr("天 ") + QTime(0,0,0).addSecs(measureStartTime.secsTo(currentDateTime) - days * 24 * 60 * 60).toString("hh:mm:ss"));
        } else{
            ui->lcdNumber->display(QTime(0,0,0).addSecs(measureStartTime.secsTo(currentDateTime)).toString("hh:mm:ss"));
        }
    });

    //注意这一条仅仅是当前应付技术规格书中，并没有实际用处。第9条：预设程宇可进行上下电，开关机等硬件操作。
    QTimer* coolingTimer_auto = new QTimer(this); //用于自动测量的冷却时间
    coolingTimer_auto->setObjectName("coolingTimer_auto");
    connect(coolingTimer_auto, &QTimer::timeout, this, [=](){
        coolingTimer_auto->stop();
        qInfo().noquote()<<"探测器已经上电开机";
        ui->lineEdit_autoStatus->setText("测量中...");
    });

    QTimer* exceptionCheckTimer = new QTimer(this);
    exceptionCheckTimer->setObjectName("exceptionCheckTimer");
    connect(exceptionCheckTimer, &QTimer::timeout, this, [=](){
        // 获取当前时间
        QDateTime now = QDateTime::currentDateTime();
        if (this->lastRecvDataTime.secsTo(now) > 1){
            //exceptionCheckTimer->stop();
            //emit sigAppengMsg(tr("探测器数据异常"), QtCriticalMsg);
            PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
            plotWidget->slotUpdatePlotNullData(commandHelper->readTimeStep());
            this->lastRecvDataTime = now;
        }
    });
    exceptionCheckTimer->stop();

    QTimer::singleShot(500, this, [=](){
       this->showMaximized();
    });

    //每次启动，先默认上次软件是正常退出
    this->setProperty("last_safe_exit", true);
    this->load();

    connect(ui->spinBox_1_leftE, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateEnWindow()));
    connect(ui->spinBox_1_rightE, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateEnWindow()));
    connect(ui->spinBox_2_leftE, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateEnWindow()));
    connect(ui->spinBox_2_rightE, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateEnWindow()));

    connect(ui->spinBox_1_leftE_2, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateEnWindow()));
    connect(ui->spinBox_1_rightE_2, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateEnWindow()));
    connect(ui->spinBox_2_leftE_2, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateEnWindow()));
    connect(ui->spinBox_2_rightE_2, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateEnWindow()));

    // 注册全局快捷键
    RegisterHotKey(reinterpret_cast<HWND>(this->winId()), 1, 0x00, VK_F1);
    RegisterHotKey(reinterpret_cast<HWND>(this->winId()), 2, 0x00, VK_F2);

    slotUpdateEnWindow();    
    // emit sigAppengMsg(QObject::tr("系统启动"), QtInfoMsg);
}

void MainWindow::slotUpdateEnWindow()
{
    unsigned short EnWin[4] = {(unsigned short)ui->spinBox_1_leftE->value(), (unsigned short)ui->spinBox_1_rightE->value(),
                               (unsigned short)ui->spinBox_2_leftE->value(), (unsigned short)ui->spinBox_2_rightE->value()};
    if (ui->tabWidget_measure->currentIndex() == 0){
        EnWin[0] = (unsigned short)ui->spinBox_1_leftE->value();
        EnWin[1] = (unsigned short)ui->spinBox_1_rightE->value();
        EnWin[2] = (unsigned short)ui->spinBox_2_leftE->value();
        EnWin[3] = (unsigned short)ui->spinBox_2_rightE->value();
    } else if (ui->tabWidget_measure->currentIndex() == 1){
        EnWin[0] = (unsigned short)ui->spinBox_1_leftE_2->value();
        EnWin[1] = (unsigned short)ui->spinBox_1_rightE_2->value();
        EnWin[2] = (unsigned short)ui->spinBox_2_leftE_2->value();
        EnWin[3] = (unsigned short)ui->spinBox_2_rightE_2->value();
    }

    PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
    plotWidget->slotUpdateEnWindow(EnWin);
}

#include <QSplitter>
void MainWindow::initCustomPlot()
{
    //this->setDockNestingEnabled(true);
    {
        PlotWidget *customPlotWidget = new PlotWidget(this);
        customPlotWidget->setObjectName("online-PlotWidget");
        customPlotWidget->initCustomPlot();
        connect(customPlotWidget, &PlotWidget::sigUpdateEnWindow, this, [=](QString name, unsigned int leftEn, unsigned int rightEn){
            if (name == "Energy-Det1")
                currentDetectorIndex = 0;
            else
                currentDetectorIndex = 1;

            if (currentDetectorIndex == 0){
                ui->spinBox_1_leftE->setValue(leftEn);
                ui->spinBox_1_rightE->setValue(rightEn);
            } else{
                ui->spinBox_2_leftE->setValue(leftEn);
                ui->spinBox_2_rightE->setValue(rightEn);
            }
        });

        connect(customPlotWidget, &PlotWidget::sigPausePlot, this, [=](bool pause){
            this->setProperty("pause_plot", pause);
            if (this->property("pause_plot").toBool()){
                ui->action_refresh->setIcon(QIcon(":/resource/work.png"));
                ui->action_refresh->setText(tr("恢复刷新"));
                ui->action_refresh->setIconText(tr("恢复刷新"));
            } else {
                this->setProperty("pause_plot", false);
                ui->action_refresh->setIcon(QIcon(":/resource/pause.png"));
                ui->action_refresh->setText(tr("暂停刷新"));
                ui->action_refresh->setIconText(tr("暂停刷新"));

                ui->pushButton_confirm->setIcon(QIcon());
            }
        });

        connect(customPlotWidget, &PlotWidget::sigAreaSelected, this, [=](){
            QWhatsThis::showText(ui->pushButton_confirm->mapToGlobal(ui->pushButton_confirm->geometry().bottomRight()), tr("需点击确认按钮，拟合区域才会设置生效"), ui->pushButton_confirm);
            ui->pushButton_confirm->setIcon(QIcon(":/resource/warn.png"));

            //勾选高斯拟合
            ui->checkBox_gauss->setChecked(true);
        });

        connect(customPlotWidget, &PlotWidget::sigSwitchToTipMode, this, [=](){
            ui->action_tip->setChecked(true);
            customPlotWidget->setProperty("Plot_Opt_model", ui->action_tip->objectName());
        });
        connect(customPlotWidget, &PlotWidget::sigSwitchToDragMode, this, [=](){
            ui->action_drag->setChecked(true);
            customPlotWidget->setProperty("Plot_Opt_model", ui->action_drag->objectName());
        });
        connect(customPlotWidget, &PlotWidget::sigSwitchToMoveMode, this, [=](){
            ui->action_move->setChecked(true);
            customPlotWidget->setProperty("Plot_Opt_model", ui->action_move->objectName());
        });

        ui->widget_plot->layout()->addWidget(customPlotWidget);
    }
}

void MainWindow::on_action_devices_triggered()
{
    EquipmentManagementForm *w = new EquipmentManagementForm(this);
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    w->setWindowFlags(w->windowFlags() | Qt::Dialog);
    w->setWindowModality(Qt::ApplicationModal);
    w->showNormal();
}

void MainWindow::slotAppendMsg(const QString &msg, QtMsgType msgType)
{
    // 创建一个 QTextCursor
    QTextCursor cursor = ui->textEdit_log->textCursor();
    // 将光标移动到文本末尾
    cursor.movePosition(QTextCursor::End);

    // 先插入时间
    cursor.insertHtml(QString("<span style='color:black;'>%1</span>").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz >> ")));
    // 再插入文本
    if (msgType == QtDebugMsg || msgType == QtInfoMsg)
        cursor.insertHtml(QString("<span style='color:black;'>%1</span>").arg(msg));
    else if (msgType == QtCriticalMsg || msgType == QtFatalMsg)
        cursor.insertHtml(QString("<span style='color:red;'>%1</span>").arg(msg));
    else
        cursor.insertHtml(QString("<span style='color:green;'>%1</span>").arg(msg));

    // 最后插入换行符
    cursor.insertHtml("<br>");

    // 确保 QTextEdit 显示了光标的新位置
    ui->textEdit_log->setTextCursor(cursor);
}

void MainWindow::on_action_SpectrumModel_triggered()
{
    SpectrumModel *w = new SpectrumModel(this);
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    w->setWindowFlags(Qt::WindowMinimizeButtonHint|Qt::WindowCloseButtonHint|Qt::Dialog);
    w->setWindowModality(Qt::ApplicationModal);
    w->showNormal();
}

/**
 * @description: 数据查看和分析
 * @return {*}
 */
void MainWindow::on_action_DataAnalysis_triggered()
{
    //防止多次创建
    if (nullptr == offlineDataAnalysisWidget){
        offlineDataAnalysisWidget = new OfflineDataAnalysisWidget(this);
        // connect(offlineDataAnalysisWidget, &OfflineDataAnalysisWidget::sigPausePlot, this, [=](bool pause){
        //     this->setProperty("pause_plot", pause);
        //     if (this->property("pause_plot").toBool()){
        //         ui->action_refresh->setIcon(QIcon(":/resource/work.png"));
        //         ui->action_refresh->setText(tr("恢复刷新"));
        //         ui->action_refresh->setIconText(tr("恢复刷新"));
        //     } else {
        //         // this->setProperty("pause_plot", false);
        //         // ui->action_refresh->setIcon(QIcon(":/resource/pause.png"));
        //         // ui->action_refresh->setText(tr("暂停刷新"));
        //         // ui->action_refresh->setIconText(tr("暂停刷新"));

        //         // ui->pushButton_confirm->setIcon(QIcon());
        //     }
        // });

        PlotWidget* plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");        
        connect(plotWidget, &PlotWidget::sigSwitchToTipMode, this, [=](){
            ui->action_tip->setChecked(true);
            plotWidget->setProperty("Plot_Opt_model", ui->action_tip->objectName());
        });
        connect(plotWidget, &PlotWidget::sigSwitchToDragMode, this, [=](){
            ui->action_drag->setChecked(true);
            plotWidget->setProperty("Plot_Opt_model", ui->action_drag->objectName());
        });
        connect(plotWidget, &PlotWidget::sigSwitchToMoveMode, this, [=](){
            ui->action_move->setChecked(true);
            plotWidget->setProperty("Plot_Opt_model", ui->action_move->objectName());
        });
        plotWidget->switchToMoveMode();

        int index = ui->tabWidget_client->addTab(offlineDataAnalysisWidget, tr("数据查看和分析"));
        ui->tabWidget_client->setCurrentIndex(index);
    }

    //防止重复打开文件，只允许打开一个窗口。
    /*if (!this->property("offline-filename").isValid() || this->property("offline-filename").toString().isEmpty()){
        QString filePath = QFileDialog::getOpenFileName(this, tr("打开文件"),"",tr("能谱文件 (*.dat)"));
        if (filePath.isEmpty() || !QFileInfo::exists(filePath))
            return;

        this->setProperty("offline-filename", filePath);
        if (offlineDataAnalysisWidget && offlineDataAnalysisWidget->isVisible()){
            offlineDataAnalysisWidget->openEnergyFile(filePath);
            emit offlineDataAnalysisWidget->sigStart();
            return ;
        }
    } else {
        ui->tabWidget_client->setCurrentWidget(offlineDataAnalysisWidget);
    }
*/
    if (!this->property("offline-filename").toString().isEmpty())
        this->setWindowTitle(this->property("offline-filename").toString() + " - Cu_Activation");
    else
        this->setWindowTitle("Cu_Activation");
}

#include "waveformmodel.h"
void MainWindow::on_action_WaveformModel_triggered()
{   
    WaveformModel *w = new WaveformModel(this);
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    w->setWindowFlags(Qt::WindowMinimizeButtonHint|Qt::WindowCloseButtonHint|Qt::Dialog);
    w->setWindowModality(Qt::ApplicationModal);
    w->showNormal();
}

// 连接外设
void MainWindow::on_action_displacement_triggered()
{
    if (this->property("test").isValid() && this->property("test").toBool()){
        this->setProperty("control_fault", false);
        if (this->property("control_on").toBool()){
            this->setProperty("control_on", false);
            this->setProperty("axis-prepared", false);
        }
        else{
            this->setProperty("control_on", true);
            this->setProperty("axis-prepared", true);
        }
        slotRefreshUi();
        return;
    }

    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::information(this, tr("提示"), tr("请先配置远程设备信息！"));
        return;
    }

    //断开连接之前先关闭电源
    // if (this->property("control_on").toBool()){
    //     commandHelper->closeDetector();
    //     commandHelper->closeRelay();
    // }

    //位移平台
    if (!this->property("control_on").toBool()){
        if (controlHelper->open_tcp()){
            //controlHelper->single_home(0x01);
            //controlHelper->single_home(0x02);

            // QPair<float, float> pair = controlHelper->gotoAbs(0);
            // this->setProperty("axis01-target-position", pair.first);
            // this->setProperty("axis02-target-position", pair.second);
        }
    } else {
        controlHelper->close();
    }
}

// 打开电源
void MainWindow::on_action_power_triggered()
{
    if (this->property("test").isValid() && this->property("test").toBool()){
        this->setProperty("relay_fault", false);
        if (this->property("relay_on").toBool())
            this->setProperty("relay_on", false);
        else
            this->setProperty("relay_on", true);
        slotRefreshUi();
        return;
    }

    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::information(this, tr("提示"), tr("请先配置远程设备信息！"));
        return;
    }

    if (!this->property("relay_on").toBool()){
        commandHelper->openRelay();
    } else {
        //如果正在测量，停止测量
        if (this->property("measure-status").toUInt() == msPrepare
                || this->property("measure-status").toUInt() == msWaiting
                || this->property("measure-status").toUInt() == msStart){
            if (this->property("measure-model").toUInt() == mmManual)
                commandHelper->slotStopManualMeasure();
            else
                commandHelper->slotStopAutoMeasure();

            commandHelper->closeDetector();
        }
        commandHelper->closeRelay();
    }
}

// 打开探测器
void MainWindow::on_action_detector_connect_triggered()
{
    if (!this->property("detector_on").toBool()){
        commandHelper->openDetector();
    } else {
        commandHelper->slotStopManualMeasure();
        commandHelper->closeDetector();
    }
}

void MainWindow::on_action_energycalibration_triggered()
{
    EnergyCalibrationForm *w = new EnergyCalibrationForm(this);
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    w->setWindowFlags(Qt::WindowMinMaxButtonsHint|Qt::WindowCloseButtonHint|Qt::Dialog);
    w->setWindowModality(Qt::ApplicationModal);
    w->showNormal();
}

void MainWindow::on_pushButton_measure_clicked()
{
    if (this->property("measure-status").toUInt() == msNone
            || this->property("measure-status").toUInt() == msEnd){
        this->saveConfigJson();

        /*CoolingTimeWidget *w = new CoolingTimeWidget(this);
        w->setAttribute(Qt::WA_DeleteOnClose, true);
        w->setWindowFlags(Qt::WindowCloseButtonHint|Qt::Dialog);
        w->setWindowModality(Qt::ApplicationModal);
        w->showNormal();
        emit w->sigUpdateTimeLength(ui->spinBox_coolingTime->value());
        connect(w, &CoolingTimeWidget::sigAskTimeLength, this, [=](int new_timelength){
            
            ui->spinBox_coolingTime->setValue(new_timelength);
        */
       {
            this->setProperty("measure-status", msPrepare);

            //手动测量
            DetectorParameter detectorParameter;
            detectorParameter.triggerThold1 = 0x81;
            detectorParameter.triggerThold2 = 0x81;
            detectorParameter.waveformPolarity = 0x00;
            detectorParameter.deadTime = 0x0A;
            detectorParameter.gain = 0x00;
            detectorParameter.measureRange = ui->comboBox_range->currentIndex()+1; //注意：从1开始计数

            // 默认打开梯形成形
            detectorParameter.isTrapShaping = true;
            detectorParameter.TrapShape_risePoint = 20;
            detectorParameter.TrapShape_peakPoint = 20;
            detectorParameter.TrapShape_fallPoint = 20;
            detectorParameter.TrapShape_constTime1 = 63150;
            detectorParameter.TrapShape_constTime2 = 62259;
            detectorParameter.TrapShape_baseLine = 20;

            detectorParameter.transferModel = 0x05;// 0x00-能谱 0x03-波形 0x05-符合模式
            detectorParameter.measureModel = mmManual;
            detectorParameter.coolingTime = ui->spinBox_coolingTime->value();
            detectorParameter.delayTime = ui->spinBox_delayTime->value();
            detectorParameter.timeWidth = ui->spinBox_timeWidth->value();
            this->setProperty("measur-model", detectorParameter.measureModel);

            // 打开 JSON 文件
            QFile file(QApplication::applicationDirPath() + "/config/fpga.json");
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                // 读取文件内容
                QByteArray jsonData = file.readAll();
                file.close(); //释放资源

                // 将 JSON 数据解析为 QJsonDocument
                QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
                QJsonObject jsonObj = jsonDoc.object();
                detectorParameter.triggerThold1 = jsonObj["TriggerThold1"].toInt();
                detectorParameter.triggerThold2 = jsonObj["TriggerThold2"].toInt();
                detectorParameter.waveformPolarity = jsonObj["WaveformPolarity"].toInt();
                detectorParameter.deadTime = jsonObj["DeadTime"].toInt();
                detectorParameter.gain = jsonObj["DetectorGain"].toInt();
            }

            // ui->pushButton_save->setEnabled(false);
            ui->action_refresh->setEnabled(true);
            ui->pushButton_measure->setEnabled(false);

            ui->pushButton_measure_2->setEnabled(false);
            int stepT = ui->spinBox_step->value();
            unsigned short EnWin[4] = {(unsigned short)ui->spinBox_1_leftE->value(), (unsigned short)ui->spinBox_1_rightE->value(),
                                       (unsigned short)ui->spinBox_2_leftE->value(), (unsigned short)ui->spinBox_2_rightE->value()};

            QTimer* measureTimer = this->findChild<QTimer*>("measureTimer");
            measureTimer->setInterval(ui->spinBox_timelength->value() * 1000);
            
            // 获取多道道数
            bool ok;
            int value = ui->comboBox_channel->currentText().toInt(&ok);
            multi_CHANNEL = static_cast<unsigned int>(value);

            PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
            
            // 开始测量时取消勾选高斯拟合，因为刚开始统计涨落大，无法拟合。
            ui->checkBox_gauss->setChecked(false);

            plotWidget->slotStart(multi_CHANNEL);            
            plotWidget->slotUpdateEnWindow(EnWin);

            ui->lcdNumber_CountRate1->display("0");
            ui->lcdNumber_CountRate2->display("0");
            ui->lcdNumber_ConCount_single->display("0");
            ui->lcdNumber_DeathRatio1->display("0.0");
            ui->lcdNumber_DeathRatio2->display("0.0");
            ui->lineEdit_CuActive->setText("0");
            ui->lineEdit_Yield->setText("0");

            ui->action_drag->setEnabled(true);
            commandHelper->setShotNumber(ui->lineEdit_ShotNum->text()); //设置测量发次，QString类型
            commandHelper->updateParamter(stepT, EnWin, false);
            commandHelper->slotStartManualMeasure(detectorParameter);

            QTimer::singleShot(1000, this, [=](){
                //指定时间未收到开始测量指令，则按钮恢复初始状态
                if (this->property("measure-status").toUInt() == msPrepare){
                    commandHelper->slotStopManualMeasure();
                    ui->pushButton_measure->setEnabled(true);
                }
            });
        }
        // });

    } else {
        ui->pushButton_measure->setEnabled(false);
        commandHelper->slotStopManualMeasure();

        QTimer::singleShot(3000, this, [=](){
            //指定时间未收到停止测量指令，则按钮恢复初始状态
            if (this->property("measure-status").toUInt() != msEnd){
                ui->pushButton_measure->setEnabled(true);
            }
        });
    }
}

void MainWindow::on_pushButton_measure_2_clicked()
{
    //自动测量
    if (this->property("measure-status").toUInt() == msNone
            || this->property("measure-status").toUInt() == msEnd){
        this->saveConfigJson();

        this->setProperty("measure-status", msPrepare);

        //自动测量
        DetectorParameter detectorParameter;
        detectorParameter.triggerThold1 = 0x81;
        detectorParameter.triggerThold2 = 0x81;
        detectorParameter.waveformPolarity = 0x00;
        detectorParameter.deadTime = 0x05*10;
        detectorParameter.gain = 0x00;
        detectorParameter.measureRange = ui->comboBox_range_2->currentIndex()+1; //注意：从1开始计数
        // 冷却时间，单位s。
        // 对于自动测量，这个前面的冷却时长内FPGA在工作，并上传数据，但是软件不对数据处理，直接丢弃。
        detectorParameter.coolingTime = ui->spinBox_coolingTime_2->value(); 
        detectorParameter.delayTime = ui->spinBox_delayTime_2->value(); //延迟时间,单位ns
        detectorParameter.timeWidth = ui->spinBox_timeWidth_2->value(); //时间窗宽度,单位ns

        // 默认打开梯形成形
        detectorParameter.isTrapShaping = true;
        detectorParameter.TrapShape_risePoint = 20;
        detectorParameter.TrapShape_peakPoint = 20;
        detectorParameter.TrapShape_fallPoint = 20;
        detectorParameter.TrapShape_constTime1 = 63150;
        detectorParameter.TrapShape_constTime2 = 62259;
        detectorParameter.TrapShape_baseLine = 20;

        detectorParameter.transferModel = 0x05;// 0x00-能谱 0x03-波形 0x05-符合模式
        detectorParameter.measureModel = mmAuto;

        this->setProperty("measur-model", detectorParameter.measureModel);

        // 打开 JSON 文件
        QFile file(QApplication::applicationDirPath() + "/config/fpga.json");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // 读取文件内容
            QByteArray jsonData = file.readAll();
            file.close(); //释放资源

            // 将 JSON 数据解析为 QJsonDocument
            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
            QJsonObject jsonObj = jsonDoc.object();
            detectorParameter.triggerThold1 = jsonObj["TriggerThold1"].toInt();
            detectorParameter.triggerThold2 = jsonObj["TriggerThold2"].toInt();
            detectorParameter.waveformPolarity = jsonObj["WaveformPolarity"].toInt();
            detectorParameter.deadTime = jsonObj["DeadTime"].toInt();
            detectorParameter.gain = jsonObj["DetectorGain"].toInt();
        }

        ui->action_refresh->setEnabled(true);
        ui->pushButton_measure->setEnabled(false);
        ui->pushButton_measure_2->setText(tr("停止测量"));
        ui->lineEdit_autoStatus->setText("等待触发");
        qInfo().noquote()<<"等待触发";

        PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
        // 开始测量时取消勾选高斯拟合，因为刚开始统计涨落大，无法拟合。
        ui->checkBox_gauss->setChecked(false);
        
        plotWidget->slotStart(multi_CHANNEL);
        plotWidget->areaSelectFinished();//直接禁用自动测量的高斯拟合功能
        
        ui->lcdNumber_CountRate1->display("0");
        ui->lcdNumber_CountRate2->display("0");
        ui->lcdNumber_ConCount_single->display("0");
        ui->lcdNumber_DeathRatio1->display("0.0");
        ui->lcdNumber_DeathRatio2->display("0.0");
        ui->lineEdit_CuActive->setText("0");
        ui->lineEdit_Yield->setText("0");        
        ui->action_drag->setEnabled(false);

        int stepT = ui->spinBox_step_2->value();
        unsigned short EnWin[4] = {(unsigned short)ui->spinBox_1_leftE_2->value(), (unsigned short)ui->spinBox_1_rightE_2->value(),
                                   (unsigned short)ui->spinBox_2_leftE_2->value(), (unsigned short)ui->spinBox_2_rightE_2->value()};

        QTimer* measureTimer = this->findChild<QTimer*>("measureTimer");
        measureTimer->setInterval(ui->spinBox_timelength_2->value() * 1000);
        // 获取多道道数
        bool ok;
        int value = ui->comboBox_channel2->currentText().toInt(&ok);
        multi_CHANNEL = static_cast<unsigned int>(value);
        
        commandHelper->setShotNumber(ui->lineEdit_ShotNum_2->text()); //设置测量序号，QString类型
        commandHelper->updateParamter(stepT, EnWin, true);
        commandHelper->slotStartAutoMeasure(detectorParameter);

        // ui->pushButton_measure_2_tip->setText(tr("等待触发..."));
        // qInfo().noquote()<<"等待触发...";
        QTimer::singleShot(1000, this, [=](){
            //指定时间未收到开始测量指令，则按钮恢复初始状态
            if (this->property("measure-status").toUInt() == msPrepare){
                commandHelper->slotStopAutoMeasure();
                ui->pushButton_measure_2->setEnabled(true);
            }
        });
    } else {
        ui->pushButton_measure_2->setEnabled(false);
        commandHelper->slotStopAutoMeasure();

        QTimer::singleShot(3000, this, [=](){
            //指定时间未收到停止测量指令，则按钮恢复初始状态
            if (this->property("measure-status").toUInt() != msEnd){
                ui->pushButton_measure_2->setEnabled(true);
            }
        });
    }
}
/*
void MainWindow::on_pushButton_save_clicked()
{
    this->saveConfigJson();

    //存储文件名
    QString filename = ui->lineEdit_path->text() + "/" + ui->lineEdit_filename->text();
    QFileInfo fInfo(filename);
    if (fInfo.exists()){
        if (QMessageBox::question(this, tr("提示"), tr("保存文件名已经存在，是否覆盖重写？"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
            return ;
    }

    // 保存参数    
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/user.json");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QString path, filename;
        path = ui->lineEdit_path->text();
        filename = ui->lineEdit_filename->text();
        if (!filename.endsWith(".dat"))
            filename += ".dat";

        QJsonObject jsonObj;
        jsonObj["path"] = path;
        jsonObj["filename"] = filename;
        QJsonDocument jsonDoc(jsonObj);
        file.write(jsonDoc.toJson());
        file.close();

        commandHelper->exportFile(path + "/" + filename);
    }
}*/

void MainWindow::on_pushButton_refresh_clicked()
{
    this->saveConfigJson();

    int stepT = ui->spinBox_step->value();

    commandHelper->updateStepTime(stepT);
}

void MainWindow::on_action_close_triggered()
{
    this->close();
}

void MainWindow::on_action_refresh_triggered()
{
    if (ui->tabWidget_client->currentWidget()->objectName() == "tabWidget_workLog")
        return ;

    if (ui->tabWidget_client->currentWidget()->objectName() == "OfflineDataAnalysisWidget"){
        PlotWidget* plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
        if (!this->property("pause_plot").toBool()){
            emit plotWidget->sigPausePlot(true);
        } else {
            emit plotWidget->sigPausePlot(false);
        }
    } else if (ui->tabWidget_client->currentWidget()->objectName() == "onlineDataAnalysisWidget"){
        PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
        if (!this->property("pause_plot").toBool()){
            emit plotWidget->sigPausePlot(true);
        } else {
            emit plotWidget->sigPausePlot(false);
        }
    }
}

void MainWindow::on_action_config_triggered()
{
    FPGASetting *w = new FPGASetting(this);
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    w->setWindowFlags(Qt::WindowCloseButtonHint|Qt::Dialog);
    w->setWindowModality(Qt::ApplicationModal);
    w->showNormal();
}

#include "aboutwidget.h"
void MainWindow::on_action_about_triggered()
{
    AboutWidget *w = new AboutWidget(this);
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    w->setWindowFlags(Qt::WindowCloseButtonHint|Qt::Dialog);
    w->setWindowModality(Qt::ApplicationModal);
    w->showNormal();
}

std::function<bool(const QString &,const QString &)> CopyDirectoryFiles = [](const QString &fromDir,const QString &toDir)
{
    QDir sourceDir(fromDir);
    QDir targetDir(toDir);

    if(!targetDir.exists())
    {
        if(!targetDir.mkdir(targetDir.absolutePath()))
        {
            return false;
        }
    }

    QFileInfoList list = sourceDir.entryInfoList();
    for(int i = 0; i < list.size(); i++)
    {
        QFileInfo fileInfo = list.at(i);
        if(fileInfo.fileName()=="." || fileInfo.fileName()=="..")
        {
            continue;
        }

        if(fileInfo.isDir())
        {
            if(!CopyDirectoryFiles(fileInfo.filePath(),targetDir.filePath(fileInfo.fileName())))
            {
                return false;
            }
        }
        else
        {
            if(targetDir.exists(fileInfo.fileName()))
            {
                targetDir.remove(fileInfo.fileName());        //有相同的，直接删除
            }
            if(!QFile::copy(fileInfo.filePath(),targetDir.filePath(fileInfo.fileName())))
            {
                return false;
            }
        }
    }

    return true;
};

void MainWindow::on_action_restore_triggered()
{
    if (QMessageBox::warning(this, tr("提示"), tr("是否恢复出厂设置？\n恢复出厂设置只影响软件界面的默认参数，不影响硬件的相关参数。"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)

    qInfo().noquote() << tr("恢复出厂设置");
    if (CopyDirectoryFiles("./config/default", "./config/"))
        this->load();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (this->property("measure-status").toUInt() != msEnd){
        QMessageBox::warning(this, tr("提示"), tr("测量过程中，禁止退出程序！"), QMessageBox::Ok, QMessageBox::Ok);
        event->ignore();
        return ;
    }

    qInstallMessageHandler(nullptr);
    slotSafeExit();

    event->accept();
}

void MainWindow::slotSafeExit()
{
    commandHelper->closeDetector();
    commandHelper->closeRelay();
    controlHelper->close();


}

#include "cachedirconfigwidget.h"
void MainWindow::on_action_cachepath_triggered()
{
    CacheDirConfigWidget *w = new CacheDirConfigWidget(this);
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    w->setWindowFlags(Qt::WindowCloseButtonHint|Qt::Dialog);
    w->setWindowModality(Qt::ApplicationModal);
    w->showNormal();
}

void MainWindow::on_action_yieldcalibration_triggered()
{
    YieldCalibration *w = new YieldCalibration(this);
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    w->setWindowFlags(Qt::WindowCloseButtonHint|Qt::Dialog);
    w->setWindowModality(Qt::ApplicationModal);
    w->showNormal();
}

void MainWindow::on_action_log_triggered()
{
    ui->widget_log->show();
}

void MainWindow::slotRefreshUi()
{
    //外设
    if (this->property("control_fault").toBool()){
        ui->action_displacement->setIcon(QIcon(":/resource/lianjie-fault.png"));
        ui->action_displacement->setText(tr("打开外设"));
        ui->action_displacement->setIconText(tr("打开外设"));
    } else {
        if (!this->property("control_on").toBool()){
            ui->action_displacement->setIcon(QIcon(":/resource/lianjie.png"));
            ui->action_displacement->setText(tr("打开外设"));
            ui->action_displacement->setIconText(tr("打开外设"));
        } else {
            ui->action_power->setEnabled(true);

            ui->action_displacement->setIcon(QIcon(":/resource/lianjie-on.png"));
            ui->action_displacement->setText(tr("关闭外设"));
            ui->action_displacement->setIconText(tr("关闭外设"));
        }
    }

    // 电源
    if (this->property("relay_fault").toBool()){
        ui->action_power->setIcon(QIcon(":/resource/power-fault.png"));
        ui->action_power->setText(tr("打开电源"));
        ui->action_power->setIconText(tr("打开电源"));
    } else {
        if (!this->property("relay_on").toBool()){
            ui->action_power->setIcon(QIcon(":/resource/power-off.png"));
            ui->action_power->setText(tr("打开电源"));
            ui->action_power->setIconText(tr("打开电源"));

            ui->action_detector_connect->setEnabled(false);
            if (!this->property("control_on").toBool()){
                ui->action_power->setEnabled(false);
            }
        } else {
            ui->action_power->setIcon(QIcon(":/resource/power-on.png"));
            ui->action_power->setText(tr("关闭电源"));
            ui->action_power->setIconText(tr("关闭电源"));

            ui->action_detector_connect->setEnabled(true);
        }
    }

    //探测器
    if (this->property("detector_fault").toBool()){
        ui->action_detector_connect->setIcon(QIcon(":/resource/conect-fault.png"));
        ui->action_detector_connect->setText(tr("打开探测器"));
        ui->action_detector_connect->setIconText(tr("打开探测器"));

        //ui->action_config->setEnabled(false);
    } else {
        if (this->property("relay_on").toBool() && !this->property("detector_on").toBool()){
            //ui->action_partical->setEnabled(false);
            //ui->action_SpectrumModel->setEnabled(false);
            //ui->action_WaveformModel->setEnabled(false);
            //ui->action_config->setEnabled(false);
            ui->action_detector_connect->setIcon(QIcon(":/resource/conect.png"));
            ui->action_detector_connect->setText(tr("打开探测器"));
            ui->action_detector_connect->setIconText(tr("打开探测器"));
        } else {
            //ui->action_partical->setEnabled(true);
            //ui->action_SpectrumModel->setEnabled(true);
            //ui->action_WaveformModel->setEnabled(true);
            //ui->action_config->setEnabled(true);

            ui->action_detector_connect->setIcon(QIcon(":/resource/conect-on.png"));
            ui->action_detector_connect->setText(tr("关闭探测器"));
            ui->action_detector_connect->setIconText(tr("关闭探测器"));
        }
    }

    //测量
    if (this->property("measure-status").toUInt() == msStart){
        ui->action_refresh->setEnabled(true);        
        // ui->pushButton_measure_2_tip->setText("");

        ui->comboBox_range->setEnabled(false);
        ui->comboBox_range_2->setEnabled(false);
        if (this->property("measur-model").toInt() == mmManual){//手动测量
            ui->pushButton_measure->setText(tr("停止测量"));
            ui->pushButton_measure->setEnabled(true);
            ui->pushButton_refresh->setEnabled(true);
            ui->pushButton_confirm->setEnabled(true);

            ui->spinBox_timelength->setEnabled(false);
            ui->comboBox_channel->setEnabled(false);
            ui->spinBox_coolingTime->setEnabled(false);
            ui->lineEdit_ShotNum->setEnabled(false);

            ui->action_power->setEnabled(false);
            ui->action_detector_connect->setEnabled(false);

            //测量过程中不允许修改能窗幅值
            ui->spinBox_1_leftE->setEnabled(true);
            ui->spinBox_1_rightE->setEnabled(true);
            ui->spinBox_2_leftE->setEnabled(true);
            ui->spinBox_2_rightE->setEnabled(true);
            ui->spinBox_1_leftE_2->setEnabled(false);
            ui->spinBox_1_rightE_2->setEnabled(false);
            ui->spinBox_2_leftE_2->setEnabled(false);
            ui->spinBox_2_rightE_2->setEnabled(false);

            // 测量中禁用符合分辨时间
            ui->spinBox_timeWidth->setEnabled(false);
            ui->spinBox_delayTime->setEnabled(false);

            //公共
            // ui->pushButton_save->setEnabled(false);

            //自动测量
            ui->pushButton_measure_2->setEnabled(false);
            ui->pushButton_refresh_2->setEnabled(false);

        } else if (this->property("measur-model").toInt() == mmAuto){//自动测量
            //测量过程中不允许修改能窗幅值
            ui->spinBox_1_leftE->setEnabled(false);
            ui->spinBox_1_rightE->setEnabled(false);
            ui->spinBox_2_leftE->setEnabled(false);
            ui->spinBox_2_rightE->setEnabled(false);
            ui->spinBox_1_leftE_2->setEnabled(false);
            ui->spinBox_1_rightE_2->setEnabled(false);
            ui->spinBox_2_leftE_2->setEnabled(false);
            ui->spinBox_2_rightE_2->setEnabled(false);

            ui->pushButton_measure_2->setText(tr("停止测量"));
            ui->pushButton_measure_2->setEnabled(true);
            
            ui->spinBox_timelength_2->setEnabled(false);
            ui->spinBox_timeWidth_2->setEnabled(false);
            ui->spinBox_delayTime_2->setEnabled(false);
            ui->comboBox_channel2->setEnabled(false);
            ui->spinBox_coolingTime_2->setEnabled(false);
            ui->lineEdit_ShotNum_2->setEnabled(false);

            ui->action_power->setEnabled(false);
            ui->action_detector_connect->setEnabled(false);

            //公共
            // ui->pushButton_save->setEnabled(false);

            //手动测量
            ui->pushButton_measure->setEnabled(false);
            ui->pushButton_refresh->setEnabled(false);
            ui->pushButton_confirm->setEnabled(false);
        }
    } else {
        ui->pushButton_measure->setText(tr("开始测量"));
        ui->pushButton_measure_2->setText(tr("开始测量"));
        
        ui->lineEdit_autoStatus->setText(tr("已停止测量"));
        
        if(this->property("measur-model").toInt() == mmAuto) //自动测量
        {
            //如果提前点击停止测量，则直接停止冷却时长的定时器
            QTimer* coolingTimer_auto = this->findChild<QTimer*>("coolingTimer_auto");
            coolingTimer_auto->stop();
            qInfo().noquote()<<"探测器已断电，并关机";
        }

        // 手动测量
        ui->spinBox_timelength->setEnabled(true);
        ui->comboBox_channel->setEnabled(true);
        ui->comboBox_range->setEnabled(true);
        ui->spinBox_coolingTime->setEnabled(true);
        ui->lineEdit_ShotNum->setEnabled(true);
        ui->spinBox_1_leftE->setEnabled(true);
        ui->spinBox_1_rightE->setEnabled(true);
        ui->spinBox_2_leftE->setEnabled(true);
        ui->spinBox_2_rightE->setEnabled(true);        
        ui->spinBox_timeWidth->setEnabled(true);
        ui->spinBox_delayTime->setEnabled(true);

        // 自动测量
        ui->spinBox_timelength_2->setEnabled(true);
        ui->comboBox_channel2->setEnabled(true);
        ui->comboBox_range_2->setEnabled(true);
        ui->spinBox_coolingTime_2->setEnabled(true);
        ui->lineEdit_ShotNum_2->setEnabled(true);
        ui->spinBox_1_leftE_2->setEnabled(true);
        ui->spinBox_1_rightE_2->setEnabled(true);
        ui->spinBox_2_leftE_2->setEnabled(true);
        ui->spinBox_2_rightE_2->setEnabled(true);
        ui->spinBox_timeWidth_2->setEnabled(true);
        ui->spinBox_delayTime_2->setEnabled(true);

        //位移平台到位才允许开始测量
        if (this->property("detector_on").toBool() && this->property("axis-prepared").toBool()){
//            ui->action_power->setEnabled(true);
//            ui->action_detector_connect->setEnabled(true);
//            ui->action_refresh->setEnabled(false);

            //公共
            // ui->pushButton_save->setEnabled(true);            

            ui->pushButton_measure->setEnabled(true);
            ui->pushButton_measure_2->setEnabled(true);
        } else {
//            ui->action_power->setEnabled(true);
//            ui->action_detector_connect->setEnabled(true);
//            ui->action_refresh->setEnabled(false);

            //公共
            // ui->pushButton_save->setEnabled(false);
            ui->pushButton_measure->setEnabled(false);
            ui->pushButton_measure_2->setEnabled(false);
        }

        ui->pushButton_refresh->setEnabled(false);
        ui->pushButton_confirm->setEnabled(false);
        ui->pushButton_refresh_2->setEnabled(false);
    }
}

void MainWindow::load()
{
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);

    QFile file(QApplication::applicationDirPath() + "/config/user.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close();

        // 将 JSON 数据解析为 QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        //手动
        QJsonObject jsonObjM1;
        if (jsonObj.contains("M1")){
            jsonObjM1 = jsonObj["M1"].toObject();
            //测量时长
            ui->spinBox_timelength->setValue(jsonObjM1["timelength"].toInt());
            //多道道数
            ui->comboBox_channel->setCurrentIndex(jsonObjM1["multiChannel_index"].toInt());
            //量程选取
            ui->comboBox_range->setCurrentIndex(jsonObjM1["range"].toInt());
            //冷却时长
            ui->spinBox_coolingTime->setValue(jsonObjM1["coolingTime"].toInt());
            //时间步长
            ui->spinBox_step->setValue(jsonObjM1["step"].toInt());
            //符合分辨时间
            ui->spinBox_timeWidth->setValue(jsonObjM1["timewidth"].toInt());

            //探测器1能窗左侧
            ui->spinBox_1_leftE->setValue(jsonObjM1["Det1_EnWidth_left"].toInt());
            //探测器1能窗右侧
            ui->spinBox_1_rightE->setValue(jsonObjM1["Det1_EnWidth_right"].toInt());
            //探测器2能窗左侧
            ui->spinBox_2_leftE->setValue(jsonObjM1["Det2_EnWidth_left"].toInt());
            //探测器2能窗右侧
            ui->spinBox_2_rightE->setValue(jsonObjM1["Det2_EnWidth_right"].toInt());
        }

        //自动
        QJsonObject jsonObjM2;
        if (jsonObj.contains("M2")){
            jsonObjM2 = jsonObj["M2"].toObject();
            //测量时长
            ui->spinBox_timelength_2->setValue(jsonObjM2["timelength"].toInt());
            //多道道数
            ui->comboBox_channel2->setCurrentIndex(jsonObjM2["multiChannel_index"].toInt());
            //量程选取
            ui->comboBox_range_2->setCurrentIndex(jsonObjM2["range"].toInt());
            //冷却时长
            ui->spinBox_coolingTime_2->setValue(jsonObjM2["coolingTime"].toInt());            
            //时间步长
            ui->spinBox_step_2->setValue(jsonObjM2["step"].toInt());
            //符合分辨时间
            ui->spinBox_timeWidth_2->setValue(jsonObjM2["timewidth"].toInt());
            //探测器1能窗左侧
            ui->spinBox_1_leftE_2->setValue(jsonObjM2["Det1_EnWidth_left"].toInt());
            //探测器1能窗右侧
            ui->spinBox_1_rightE_2->setValue(jsonObjM2["Det1_EnWidth_right"].toInt());
            //探测器2能窗左侧
            ui->spinBox_2_leftE_2->setValue(jsonObjM2["Det2_EnWidth_left"].toInt());
            //探测器2能窗右侧
            ui->spinBox_2_rightE_2->setValue(jsonObjM2["Det2_EnWidth_right"].toInt());
        }

        //公共
        QJsonObject jsonObjPub;
        if (jsonObj.contains("Public")){
            jsonObjPub = jsonObj["Public"].toObject();
            //保存数据
            // ui->lineEdit_path->setText(jsonObjPub["path"].toString());
            // ui->lineEdit_filename->setText(jsonObjPub["filename"].toString());
            ui->comboBox_range->setCurrentIndex(jsonObjPub["select-range"].toInt());  //量程索引值

            bool bSafeExitFlag = jsonObjPub["safe_exit"].toBool();
            this->setProperty("last_safe_exit", bSafeExitFlag);
        }
    }
}

void MainWindow::saveConfigJson(bool bSafeExitFlag)
{
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);

    QFile file(QApplication::applicationDirPath() + "/config/user.json");
    if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close();

        // 将 JSON 数据解析为 QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        //手动
        QJsonObject jsonObjM1;
        //测量时长
        jsonObjM1["timelength"] = ui->spinBox_timelength->value();
        //多道道数
        jsonObjM1["multiChannel_index"] = ui->comboBox_channel->currentIndex();
        //量程选取
        jsonObjM1["range"] = ui->comboBox_range->currentIndex();
        //冷却时长
        jsonObjM1["coolingTime"] = ui->spinBox_coolingTime->value();
        //时间步长
        jsonObjM1["step"] = ui->spinBox_step->value();
        //符合分辨时间
        jsonObjM1["timewidth"] = ui->spinBox_timeWidth->value();
        //探测器1能窗左侧
        jsonObjM1["Det1_EnWidth_left"] = ui->spinBox_1_leftE->value();
        //探测器1能窗右侧
        jsonObjM1["Det1_EnWidth_right"] = ui->spinBox_1_rightE->value();
        //探测器2能窗左侧
        jsonObjM1["Det2_EnWidth_left"] = ui->spinBox_2_leftE->value();
        //探测器2能窗右侧
        jsonObjM1["Det2_EnWidth_right"] = ui->spinBox_2_rightE->value();
        jsonObj["M1"] = jsonObjM1;

        //自动
        QJsonObject jsonObjM2;
        //测量时长
        jsonObjM2["timelength"] = ui->spinBox_timelength_2->value();
        //多道道数
        jsonObjM2["multiChannel_index"] = ui->comboBox_channel2->currentIndex();
        //量程选取
        jsonObjM2["range"] = ui->comboBox_range_2->currentIndex();
        //冷却时长
        jsonObjM2["coolingTime"] = ui->spinBox_coolingTime_2->value();
        //时间步长
        jsonObjM2["step"] = ui->spinBox_step_2->value();
        //符合分辨时间
        jsonObjM2["timewidth"] = ui->spinBox_timeWidth_2->value();
        //探测器1能窗左侧
        jsonObjM2["Det1_EnWidth_left"] = ui->spinBox_1_leftE_2->value();
        //探测器1能窗右侧
        jsonObjM2["Det1_EnWidth_right"] = ui->spinBox_1_rightE_2->value();
        //探测器2能窗左侧
        jsonObjM2["Det2_EnWidth_left"] = ui->spinBox_2_leftE_2->value();
        //探测器2能窗右侧
        jsonObjM2["Det2_EnWidth_right"] = ui->spinBox_2_rightE_2->value();
        jsonObj["M2"] = jsonObjM2;

        //公共
        QJsonObject jsonObjPub;

        //保存数据
        jsonObjPub["safe_exit"] = bSafeExitFlag;
        // jsonObjPub["path"] = ui->lineEdit_path->text();
        // jsonObjPub["filename"] = ui->lineEdit_filename->text();
        jsonObjPub["select-range"] = ui->comboBox_range->currentIndex();  //量程索引值
        jsonObj["Public"] = jsonObjPub;

        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QJsonDocument jsonDocNew(jsonObj);
        file.write(jsonDocNew.toJson());
        file.close();
    }
}

void MainWindow::on_pushButton_refresh_2_clicked()
{
    this->saveConfigJson();

    int stepT = ui->spinBox_step_2->value();

    commandHelper->updateStepTime(stepT);
}

void MainWindow::on_action_line_log_triggered()
{
    if (ui->tabWidget_client->currentWidget()->objectName() == "tabWidget_workLog")
        return ;

    if (this->property("ScaleLogarithmicType").toBool()){
        this->setProperty("ScaleLogarithmicType", false);
        PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
        if (ui->tabWidget_client->currentWidget()->objectName() == "OfflineDataAnalysisWidget"){
            plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
        }
        // plotWidget->slotResetPlot();
        plotWidget->switchToLogarithmicMode(false);

        ui->action_line_log->setText(tr("对数"));
        ui->action_line_log->setIconText(tr("对数"));
        ui->action_line_log->setIcon(QIcon(":/resource/mathlog.png"));
    } else {
        this->setProperty("ScaleLogarithmicType", true);
        PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
        if (ui->tabWidget_client->currentWidget()->objectName() == "OfflineDataAnalysisWidget"){
            plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
        }
        plotWidget->switchToLogarithmicMode(true);

        ui->action_line_log->setText(tr("线性"));
        ui->action_line_log->setIconText(tr("线性"));
        ui->action_line_log->setIcon(QIcon(":/resource/line.png"));
    }
}

#include "controlwidget.h"
//打开位移平台界面
void MainWindow::on_action_Moving_triggered()
{
    ControlWidget *w = new ControlWidget(this);
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    w->setWindowFlags(Qt::WindowCloseButtonHint|Qt::Dialog);
    w->setWindowModality(Qt::ApplicationModal);
    w->showNormal();
}

#include <QRegExp>
bool isFloat(const QString &str) {
    QRegExp re("^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$"); // 正则表达式匹配浮点数格式
    return re.exactMatch(str);
}

void MainWindow::on_lineEdit_editingFinished()
{
    QString arg1 = ui->lineEdit_Yield->text();
    if (!isFloat(arg1)){
        ui->lineEdit_Yield->clear();
        ui->lineEdit_Yield->setPlaceholderText(tr("输入数字无效"));
    }
}

void MainWindow::on_action_start_measure_triggered()
{
    if (!this->property("test").isValid()){
        this->setProperty("test", false);
    }

    if (this->property("test").toBool()){
        ui->action_start_measure->setIcon(QIcon(":/resource/circle-red.png"));
        ui->action_start_measure->setText(tr("开启离线测试模式"));
        ui->action_start_measure->setIconText(tr("开启离线测试模式"));
        this->setProperty("test", false);
    }
    else {
        ui->action_start_measure->setIcon(QIcon(":/resource/circle-green.png"));
        ui->action_start_measure->setText(tr("退出离线测试模式"));
        ui->action_start_measure->setIconText(tr("退出离线测试模式"));
        this->setProperty("test", true);
    }
}

void MainWindow::on_checkBox_gauss_stateChanged(int arg1)
{
    PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
    plotWidget->slotShowGaussInfor(arg1 == Qt::CheckState::Checked);
}

void MainWindow::on_pushButton_confirm_clicked()
{
    if (QMessageBox::question(this, tr("提示"), tr("您确定用当前能宽值来进行运算吗？\r\n\r\n提醒：整个测量过程中只允许设置能宽一次。"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
        return ;

    this->saveConfigJson();

    int stepT = ui->spinBox_step->value();
    unsigned short EnWin[4] = {(unsigned short)ui->spinBox_1_leftE->value(), (unsigned short)ui->spinBox_1_rightE->value(),
                               (unsigned short)ui->spinBox_2_leftE->value(), (unsigned short)ui->spinBox_2_rightE->value()};

    PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
    if (ui->radioButton_ref->isChecked()){
        plotWidget->slotResetPlot();
    }

    commandHelper->updateParamter(stepT, EnWin, true);


    //确认能窗后保存测量参数
    //getsetEnTime必须在commandHelper->updateParamter()调用更新后才能调用，否则返回值为0
    QString validDataFileName = commandHelper->getValidFilename();
    QString configResultFile = validDataFileName + ".配置";
    {
        QFile file(configResultFile);
        if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QTextStream out(&file);
            DetectorParameter detParameter = commandHelper->getDetParameter();
            QMap<qint8, double> gainvalue = commandHelper->getGainValue();            
            //开始保存FPGA数据的时刻，单位s，以活化后开始计时(冷却时间+FPGA时钟)。必须在
            quint32 savetime_FPGA = commandHelper->getsetEnTime();

            // 保存粒子测量参数
            out << tr("阈值1=") << detParameter.triggerThold1 << Qt::endl;
            out << tr("阈值2=") << detParameter.triggerThold2 << Qt::endl;
            out << tr("波形极性=") << ((detParameter.waveformPolarity==0x00) ? tr("正极性") : tr("负极性")) << Qt::endl;
            out << tr("死时间=") << detParameter.deadTime << Qt::endl;
            out << tr("波形触发模式=") << ((detParameter.triggerModel==0x00) ? tr("normal") : tr("auto")) << Qt::endl;
            if (gainvalue.contains(detParameter.gain))
                out << tr("探测器增益=") << gainvalue[detParameter.gain] << Qt::endl;
            switch (detParameter.measureRange){
                case 1:
                    out << tr("量程选取=小量程")<< Qt::endl;
                    break;
                case 2:
                    out << tr("量程选取=中量程")<< Qt::endl;
                    break;
                case 3:
                    out << tr("量程选取=大量程")<< Qt::endl;
                    break;
                default:
                break;
            }
            out << tr("冷却时长=") << detParameter.coolingTime << Qt::endl; //单位s

            out << tr("测量开始时间(冷却时间+FPGA时钟)=")<< savetime_FPGA <<Qt::endl;
            out << tr("符合延迟时间=") << detParameter.delayTime << Qt::endl; //单位ns
            out << tr("符合分辨时间=") << detParameter.timeWidth << Qt::endl; //单位ns
            out << tr("时间步长=") << 1 << Qt::endl; //注意，存储的数据时间步长恒为1s
            // out << tr("测量时长=") <<currentFPGATime<< Qt::endl;
            out << tr("Det1符合能窗左=") << EnWin[0] << Qt::endl;
            out << tr("Det1符合能窗右=") << EnWin[1] << Qt::endl;
            out << tr("Det2符合能窗左=") << EnWin[2] << Qt::endl;
            out << tr("Det2符合能窗右=") << EnWin[3] << Qt::endl;
            if (detParameter.measureModel == mmManual){
                //手动
                out << tr("测量模式=手动") << Qt::endl;
            } else if (detParameter.measureModel == mmAuto){
                //自动
                out << tr("测量模式=自动") << Qt::endl;
            }

            file.flush();
            file.close();
        }
    }
    qInfo().noquote() << tr("本次测量参数配置已存放在：%1").arg(configResultFile);

    //取消画面暂停刷新
    this->setProperty("pause-plot", false);
    ui->pushButton_confirm->setIcon(QIcon());

    ui->spinBox_1_leftE->setEnabled(false);
    ui->spinBox_1_rightE->setEnabled(false);
    ui->spinBox_2_leftE->setEnabled(false);
    ui->spinBox_2_rightE->setEnabled(false);
    ui->pushButton_confirm->setEnabled(false);

    QList<QAction*> actions = ui->toolBar_online->actions();
    for (auto action : actions){
        if (action->objectName() == "action_drag"){
            action->setEnabled(false);
        }
    }

    //自动切换到计数模式
    commandHelper->switchToCountMode(true);
    plotWidget->switchToCountMode(true);
    ui->radioButton_ref->setChecked(true);

    plotWidget->areaSelectFinished();
    emit plotWidget->sigPausePlot(false);
}


void MainWindow::on_action_analyze_triggered()
{
    if (ui->tabWidget_client->currentWidget()->objectName() == "OfflineDataAnalysisWidget"){
        emit offlineDataAnalysisWidget->sigStart();
    }
}


void MainWindow::on_tabWidget_client_currentChanged(int /*index*/)
{
    if (ui->tabWidget_client->currentWidget()->objectName() == "tabWidget_workLog")
        return ;

    if (ui->tabWidget_client->currentWidget()->objectName() == "OfflineDataAnalysisWidget"){
        ui->toolBar_offline->show();
        ui->toolBar_online->hide();
        if (!this->property("offline-filename").toString().isEmpty())
            this->setWindowTitle(this->property("offline-filename").toString() + " - Cu_Activation");
        else
            this->setWindowTitle("Cu_Activation");

        PlotWidget* plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
        QAction* action = this->findChild<QAction*>(plotWidget->property("Plot_Opt_model").toString());
        action->setChecked(true);
        ui->action_drag->setEnabled(true);
    } else if (ui->tabWidget_client->currentWidget()->objectName() == "onlineDataAnalysisWidget"){
        ui->toolBar_offline->hide();
        ui->toolBar_online->show();
        this->setWindowTitle("Cu_Activation");

        PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
        QAction* action = this->findChild<QAction*>(plotWidget->property("Plot_Opt_model").toString());
        action->setChecked(true);

        if (this->property("measure-status").toInt() == msStart){
            ui->action_drag->setEnabled(false);
        } else {
            ui->action_drag->setEnabled(true);
        }
    }
}

//工具栏打开文件按钮
void MainWindow::on_action_openfile_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("打开文件"),";",tr("符合测量文件 (*.dat)"));
    if (filePath.isEmpty() || !QFileInfo::exists(filePath))
        return;

    this->setProperty("offline-filename", filePath);
    offlineDataAnalysisWidget->openEnergyFile(filePath);
    this->setWindowTitle(this->property("offline-filename").toString() + " - Cu_Activation");
    emit offlineDataAnalysisWidget->sigStart();
}

#include <QDesktopServices>
void MainWindow::on_action_help_triggered()
{
    //帮助
    QString helpFilePath = QApplication::applicationDirPath() + "/软件使用说明书.pdf";

    QDesktopServices::openUrl(QUrl::fromLocalFile(helpFilePath));
}


void MainWindow::on_action_viewlog_triggered()
{
    //查看日志
    // 获取当前日期，并格式化为YYYY-MM-DD
    QString currentDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    // 创建日志文件路径，例如：logs/2023-10-23.log
    QString logFilePath = QDir::currentPath() + "/logs/Cu_Activation_" + currentDate + ".log";

    QDesktopServices::openUrl(QUrl::fromLocalFile(logFilePath));
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    MSG* msg = reinterpret_cast<MSG*>(message);
    if (msg->message == WM_HOTKEY) {
        switch (msg->wParam) {
        case 1: // F1
            emit ui->action_help->triggered();
            break;
        case 2: // F2
            emit ui->action_viewlog->triggered();
            break;
        }
        return true;
    }

    return QWidget::nativeEvent(eventType, message, result); // 传递给基类处理
}

void MainWindow::on_action_move_triggered()
{
    if (ui->tabWidget_client->currentWidget()->objectName() == "tabWidget_workLog")
        return ;

    PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
    if (ui->tabWidget_client->currentWidget()->objectName() == "OfflineDataAnalysisWidget"){
        plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
    }

    plotWidget->switchToMoveMode();
}

void MainWindow::on_action_tip_triggered()
{
    if (ui->tabWidget_client->currentWidget()->objectName() == "tabWidget_workLog")
        return ;

    PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
    if (ui->tabWidget_client->currentWidget()->objectName() == "OfflineDataAnalysisWidget"){
        plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
    }

    plotWidget->switchToTipMode();
}

void MainWindow::on_action_drag_triggered()
{
    QAction *_action = (QAction*)sender();
    if (ui->tabWidget_client->currentWidget()->objectName() == "tabWidget_workLog")
        return ;

    PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
    if (ui->tabWidget_client->currentWidget()->objectName() == "OfflineDataAnalysisWidget"){
        plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
    }

    plotWidget->switchToDragMode();
}


void MainWindow::on_action_restore_2_triggered()
{
    if (ui->tabWidget_client->currentWidget()->objectName() == "tabWidget_workLog")
        return ;

    PlotWidget* plotWidget = this->findChild<PlotWidget*>("online-PlotWidget");
    if (ui->tabWidget_client->currentWidget()->objectName() == "OfflineDataAnalysisWidget"){
        plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
    }

    plotWidget->slotRestoreView();
}
