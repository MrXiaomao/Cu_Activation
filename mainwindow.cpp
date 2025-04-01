#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "equipmentmanagementform.h"
#include "energycalibrationform.h"
#include "spectrumModel.h"
#include "dataanalysiswidget.h"
#include "plotwidget.h"
#include "FPGASetting.h"
#include "qcustomplot.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    connect(this, SIGNAL(sigRefreshUi()), this, SLOT(slotRefreshUi()));
    connect(this, SIGNAL(sigAppengMsg(const QString &, QtMsgType)), this, SLOT(slotAppendMsg(const QString &, QtMsgType)));

    commandHelper = CommandHelper::instance();
    controlHelper = ControlHelper::instance();

    //外设状态
    connect(controlHelper, &ControlHelper::sigControlFault, this, [=](){
        this->setProperty("control_fault", true);
        this->setProperty("control_on", false);

        emit sigRefreshUi();
    });

    connect(controlHelper, &ControlHelper::sigControlStatus, this, [=](bool on){
        this->setProperty("control_fault", false);
        this->setProperty("control_on", on);

        //外设连接成功，主动连接电源
        if (this->property("control_on").toBool()){
            commandHelper->openRelay();
        }

        QString msg = QString(tr("外设状态：%2")).arg(on ? tr("开") : tr("关"));
        QLabel* label_Connected = this->findChild<QLabel*>("label_Connected");
        label_Connected->setText(msg);

        emit sigAppengMsg(msg, QtInfoMsg);
        emit sigRefreshUi();
    });

    connect(controlHelper, &ControlHelper::sigReportAbs, this, [=](float f1, float f2){
        QLabel* label_axis01 = this->findChild<QLabel*>("label_axis01");
        label_axis01->setText(tr("轴1：") + QString::number(f1 / 1000, 'f', 4) + " mm");

        QLabel* label_axis02 = this->findChild<QLabel*>("label_axis02");
        label_axis02->setText(tr("轴2：") + QString::number(f2 / 1000, 'f', 4) + " mm");
    });

    // 禁用开启电源按钮
    ui->action_power->setEnabled(false);
    connect(commandHelper, &CommandHelper::sigRelayFault, this, [=](){
        this->setProperty("relay_fault", true);
        this->setProperty("relay_on", false);

        QString msg = QString(tr("网络故障，电源连接失败！"));
        emit sigAppengMsg(msg, QtCriticalMsg);
        emit sigRefreshUi();
    });

    connect(commandHelper, &CommandHelper::sigRelayStatus, this, [=](bool on){
        this->setProperty("relay_fault", false);
        this->setProperty("relay_on", on);

        if (!on){
            //电源关闭，设备断电，测量强制停止
            this->setProperty("measure-status", msEnd);
        }
        QString msg = QString(tr("电源状态：%1")).arg(on ? tr("开") : tr("关"));
        emit sigAppengMsg(msg, QtInfoMsg);
        emit sigRefreshUi();
    });

    // 自动测量开始
    connect(commandHelper, &CommandHelper::sigMeasureStart, this, [=](qint8 mode){
        if (mode == 0x02 || mode == 0x03)
            return;//波形测量、能谱测量

        lastRecvDataTime = QDateTime::currentDateTime();
        QTimer* exceptionCheckTimer = this->findChild<QTimer*>("exceptionCheckTimer");
        exceptionCheckTimer->start(5000);

        this->setProperty("measure-status", msStart);
        this->setProperty("measure-model", mode);
        if (mode == mmAuto)
            ui->start_time_text->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        QString msg = tr("测量正式开始");
        emit sigAppengMsg(msg, QtCriticalMsg);

        //开启测量时钟
        if (mode == mmManual || mode == mmAuto){//手动/自动测量
            //测量倒计时时钟
            QTimer* measureTimer = this->findChild<QTimer*>("measureTimer");
            measureTimer->start();

            //测量时长时钟
            measureStartTime = lastRecvDataTime;
            QTimer* measureRefTimer = this->findChild<QTimer*>("measureRefTimer");
            measureRefTimer->start(500);
        }

        emit sigAppengMsg(msg, QtInfoMsg);
        emit sigRefreshUi();
    });

    connect(commandHelper, &CommandHelper::sigMeasureWait, this, [=](){
        this->setProperty("measure-status", msWaiting);
    });

    // 测量停止
    connect(commandHelper, &CommandHelper::sigMeasureStop, this, [=](){
        QTimer* measureTimer = this->findChild<QTimer*>("measureTimer");
        measureTimer->stop();

        QTimer* measureRefTimer = this->findChild<QTimer*>("measureRefTimer");
        measureRefTimer->stop();

        this->setProperty("measure-status", msEnd);
        QString msg = tr("测量已停止");
        emit sigAppengMsg(msg, QtInfoMsg);
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
        emit sigAppengMsg(msg, QtCriticalMsg);
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
        emit sigAppengMsg(msg, QtInfoMsg);
        emit sigRefreshUi();
    });

    qRegisterMetaType<vector<SingleSpectrum>>("vector<SingleSpectrum>");
    qRegisterMetaType<vector<CurrentPoint>>("vector<CurrentPoint>");
    qRegisterMetaType<vector<CoincidenceResult>>("vector<CoincidenceResult>");
    std::cout << "main thread id:" << QThread::currentThreadId() << std::endl;
    connect(commandHelper, &CommandHelper::sigPlot, this, [=](vector<SingleSpectrum> r1, vector<CurrentPoint> r2, vector<CoincidenceResult> r3){
        lastRecvDataTime = QDateTime::currentDateTime();
        bool pause_plot = this->property("pause_plot").toBool();
        if (!pause_plot){
            PlotWidget* plotWidget = this->findChild<PlotWidget*>("real-PlotWidget");
            plotWidget->slotUpdatePlotDatas(r1, r2, r3);

            std::cout << "sub thread id:" << QThread::currentThreadId() << std::endl;
            ui->lcdNumber_CountRate1->display(r3.back().CountRate1);
            ui->lcdNumber_CountRate2->display(r3.back().CountRate2);
            ui->lcdNumber_ConCount_single->display(r3.back().ConCount_single);
        }
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
    // 获取当前时间
    ui->textEdit_log->clear();

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

        ui->lineEdit_path->setText(jsonObj["path"].toString());
        ui->lineEdit_filename->setText(jsonObj["filename"].toString());

        QString cacheDir = jsonObj["defaultCache"].toString();
        commandHelper->setDefaultCacheDir(cacheDir);
    } else {
        ui->lineEdit_path->setText("./");
        ui->lineEdit_filename->setText("test.dat");
        commandHelper->setDefaultCacheDir("./");
    }

    // 测量结果-存储路径
    {
        QAction *action = ui->lineEdit_path->addAction(QIcon(":/resource/open.png"), QLineEdit::TrailingPosition);
        QToolButton* button = qobject_cast<QToolButton*>(action->associatedWidgets().last());
        button->setCursor(QCursor(Qt::PointingHandCursor));
        connect(button, &QToolButton::pressed, this, [=](){
            QString dir = QFileDialog::getExistingDirectory(this);
            ui->lineEdit_path->setText(dir);
        });
    }

    ui->tabWidget_measure->setCurrentIndex(0);

    // 启用关闭按钮
    ui->tabWidget_client->setTabsClosable(true);
    ui->tabWidget_client->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);//第一个tab取消关闭按钮
    connect(ui->tabWidget_client, &QTabWidget::tabCloseRequested, this, [=](int index){
        ui->tabWidget_client->removeTab(index);
    });

    connect(ui->comboBox_range, QOverload<int>::of(&QComboBox::currentIndexChanged), ui->comboBox_range_2, &QComboBox::setCurrentIndex);
    connect(ui->comboBox_range, QOverload<int>::of(&QComboBox::currentIndexChanged), ui->comboBox_range_3, &QComboBox::setCurrentIndex);
    connect(ui->comboBox_range_2, QOverload<int>::of(&QComboBox::currentIndexChanged), ui->comboBox_range, &QComboBox::setCurrentIndex);
    connect(ui->comboBox_range_3, QOverload<int>::of(&QComboBox::currentIndexChanged), ui->comboBox_range, &QComboBox::setCurrentIndex);
    connect(ui->comboBox_range, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index){
        controlHelper->gotoAbs(index);
    });

    // 显示计数/能谱
    ui->label_tag_2->setBuddy(ui->widget_tag_2);
    ui->label_tag_2->installEventFilter(this);
    ui->label_tag_2->setStyleSheet("");
    QButtonGroup *grp = new QButtonGroup(this);
    grp->addButton(ui->radioButton_ref, 0);
    grp->addButton(ui->radioButton_spectrum, 1);
    connect(grp, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [=](int index){        
        commandHelper->switchShowModel(index == 0);
        PlotWidget* plotWidget = this->findChild<PlotWidget*>("real-PlotWidget");
        plotWidget->slotResetPlot();
        plotWidget->switchShowModel(index == 0);

        if (0 == index){
            ui->widget_gauss->hide();
        } else {
            ui->widget_gauss->show();
        }
    });
    emit grp->buttonClicked(1);//默认能谱模式

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
    int screenValidHeight = QGuiApplication::primaryScreen()->size().height();
    if (screenValidHeight <= 768){
        // 分页
        int index = ui->tabWidget_client->addTab(ui->widget_log, tr("日志视窗"));
        ui->tabWidget_client->tabBar()->setTabButton(index, QTabBar::RightSide, nullptr);//tab取消关闭按钮
    } else if (screenValidHeight <= 1080){
        // 右侧
        ui->widget_right->layout()->removeItem(ui->verticalSpacer_3);//移除弹簧
        ui->widget_right->layout()->addWidget(ui->widget_log);
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
        emit sigAppengMsg(tr("定时测量倒计时结束，自动停止测量！"), QtInfoMsg);

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

    QTimer* exceptionCheckTimer = new QTimer(this);
    exceptionCheckTimer->setObjectName("exceptionCheckTimer");
    connect(exceptionCheckTimer, &QTimer::timeout, this, [=](){
        // 获取当前时间
        QDateTime currentDateTime = QDateTime::currentDateTime();
        if (this->property("measure-status").toUInt() == msPrepare){
            if (lastRecvDataTime.secsTo(currentDateTime) > 3){
                exceptionCheckTimer->stop();
                emit sigAppengMsg(tr("探测器数据异常"), QtCriticalMsg);
                //QMessageBox::critical(this, tr("错误"), tr("探测器故障，请检查！"));
            }
        }
    });
    exceptionCheckTimer->stop();

    QTimer::singleShot(500, this, [=](){
       this->showMaximized();
    });

    this->load();
    this->slotAppendMsg(QObject::tr("系统启动"), QtInfoMsg);
}

#include <QSplitter>
void MainWindow::initCustomPlot()
{
    //this->setDockNestingEnabled(true);
    {
        PlotWidget *customPlotWidget = new PlotWidget(this);
        customPlotWidget->setObjectName("real-PlotWidget");
        customPlotWidget->initCustomPlot();
        connect(customPlotWidget, &PlotWidget::sigUpdateMeanValues, this, [=](QString name, unsigned int minMean, unsigned int maxMean){
            if (name == "Det1")
                currentDetectorIndex = 0;
            else
                currentDetectorIndex = 1;

            if (currentDetectorIndex == 0){
                ui->spinBox_1_leftE->setValue(minMean);
                ui->spinBox_1_rightE->setValue(maxMean);
            }

           ui->spinBox_leftE->setValue(minMean);
           ui->spinBox_rightE->setValue(maxMean);
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

void MainWindow::on_action_DataAnalysis_triggered()
{
    if (dataAnalysisWidget && dataAnalysisWidget->isVisible()){
        return ;
    }

    //数据解析
    if (nullptr == dataAnalysisWidget){
        dataAnalysisWidget = new DataAnalysisWidget(this);
        //dataAnalysisWidget->setAttribute(Qt::WA_DeleteOnClose, true);
        //dataAnalysisWidget->setWindowModality(Qt::ApplicationModal);
    }

    int index = ui->tabWidget_client->addTab(dataAnalysisWidget, tr("数据解析"));
    ui->tabWidget_client->setCurrentIndex(index);
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
        if (this->property("control_on").toBool())
            this->setProperty("control_on", false);
        else
            this->setProperty("control_on", true);
        slotRefreshUi();
        return;
    }

    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::information(this, tr("提示"), tr("请先配置远程设备信息！"));
        return;
    }

    //断开连接之前先关闭电源
    if (this->property("control_on").toBool()){
        commandHelper->closeDetector();
        commandHelper->closeRelay();
    }

    //位移平台
    if (!this->property("control_on").toBool()){
        if (controlHelper->open_tcp())
            controlHelper->gotoAbs(0);
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

#include "coolingtimewidget.h"
void MainWindow::on_pushButton_measure_clicked()
{
    if (this->property("measure-status").toUInt() == msNone
            || this->property("measure-status").toUInt() == msEnd){
        this->save();

        CoolingTimeWidget *w = new CoolingTimeWidget(this);
        w->setAttribute(Qt::WA_DeleteOnClose, true);
        w->setWindowFlags(Qt::WindowCloseButtonHint|Qt::Dialog);
        w->setWindowModality(Qt::ApplicationModal);
        w->showNormal();
        emit w->sigUpdateTimeLength(ui->spinBox_cool_timelength->value());
        connect(w, &CoolingTimeWidget::sigAskTimeLength, this, [=](int new_timelength){
            ui->spinBox_cool_timelength->setValue(new_timelength);

            this->setProperty("measure-status", msPrepare);

            //手动测量
            DetectorParameter detectorParameter;
            detectorParameter.triggerThold1 = 0x81;
            detectorParameter.triggerThold2 = 0x81;
            detectorParameter.waveformPolarity = 0x00;
            detectorParameter.dieTimeLength = 1;
            detectorParameter.gain = 0x00;
            detectorParameter.transferModel = 0x05;// 0x00-能谱 0x03-波形 0x05-符合模式
            detectorParameter.measureModel = mmManual;
            detectorParameter.cool_timelength = ui->spinBox_cool_timelength->value();
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
                detectorParameter.dieTimeLength = jsonObj["DieTimeLength"].toInt() / 10;
                detectorParameter.gain = jsonObj["DetectorGain"].toInt();
            }

            ui->pushButton_save->setEnabled(false);
            ui->pushButton_gauss->setEnabled(true);
            ui->action_refresh->setEnabled(true);
            ui->pushButton_measure->setEnabled(false);

            PlotWidget* plotWidget = this->findChild<PlotWidget*>("real-PlotWidget");
            plotWidget->slotResetPlot();

            ui->pushButton_measure_2->setEnabled(false);
            int stepT = ui->spinBox_step->value();
            int EnWin[4] = {ui->spinBox_1_leftE->value(), ui->spinBox_1_rightE->value(),
                                    ui->spinBox_2_leftE->value(), ui->spinBox_2_rightE->value()};
            int timewidth = ui->spinBox_timeWidth->value();
            QTimer* measureTimer = this->findChild<QTimer*>("measureTimer");
            measureTimer->setInterval(ui->spinBox_timelength_3->value() * 1000);
            commandHelper->updateParamter(stepT, EnWin, timewidth, false);
            commandHelper->slotStartManualMeasure(detectorParameter);

            QTimer::singleShot(3000, this, [=](){
                //指定时间未收到开始测量指令，则按钮恢复初始状态
                if (this->property("measure-status").toUInt() == msPrepare){
                    commandHelper->slotStopManualMeasure();
                    ui->pushButton_measure->setEnabled(true);
                }
            });
        });

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
        this->save();

        this->setProperty("measure-status", msPrepare);

        //手动测量
        DetectorParameter detectorParameter;
        detectorParameter.triggerThold1 = 0x81;
        detectorParameter.triggerThold2 = 0x81;
        detectorParameter.waveformPolarity = 0x00;
        detectorParameter.dieTimeLength = 0x05;
        detectorParameter.gain = 0x00;
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
            detectorParameter.dieTimeLength = jsonObj["DieTimeLength"].toInt();
            detectorParameter.gain = jsonObj["DetectorGain"].toInt();
        }

        ui->pushButton_save->setEnabled(false);
        ui->pushButton_gauss->setEnabled(true);
        ui->action_refresh->setEnabled(true);
        ui->pushButton_measure->setEnabled(false);
        ui->pushButton_measure_2->setText(tr("停止测量"));
        ui->start_time_text->setText("0000-00-00 00:00:00");

        PlotWidget* plotWidget = this->findChild<PlotWidget*>("real-PlotWidget");
        plotWidget->slotResetPlot();

        int stepT = ui->spinBox_step_2->value();
        int EnWin[4] = {ui->spinBox_1_leftE->value(), ui->spinBox_1_rightE->value(),
            ui->spinBox_2_leftE->value(), ui->spinBox_2_rightE->value()};
        int timewidth = ui->spinBox_timeWidth->value();
        QTimer* measureTimer = this->findChild<QTimer*>("measureTimer");
        measureTimer->setInterval(ui->spinBox_timelength_3->value() * 1000);

        commandHelper->updateParamter(stepT, EnWin, timewidth, false);
        commandHelper->slotStartAutoMeasure(detectorParameter);

        ui->pushButton_measure_2_tip->setText(tr("等待触发..."));
        QTimer::singleShot(3000, this, [=](){
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

void MainWindow::on_pushButton_measure_3_clicked()
{
    //标定测量
    this->save();
}

void MainWindow::on_pushButton_save_clicked()
{
    this->save();

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

        commandHelper->saveFileName(path + "/" + filename);
    }       
}

void MainWindow::on_pushButton_refresh_clicked()
{
    this->save();

    int stepT = ui->spinBox_step->value();
    int EnWin[4] = {ui->spinBox_1_leftE->value(), ui->spinBox_1_rightE->value(),
        ui->spinBox_2_leftE->value(), ui->spinBox_2_rightE->value()};

    if (ui->radioButton_ref->isChecked()){
        PlotWidget* plotWidget = this->findChild<PlotWidget*>("real-PlotWidget");
        plotWidget->slotResetPlot();
    }

    int timewidth = ui->spinBox_timeWidth->value();
    commandHelper->updateParamter(stepT, EnWin, timewidth, true);
}

void MainWindow::on_action_close_triggered()
{
    this->close();
}

void MainWindow::on_action_refresh_triggered()
{
    if (!this->property("pause_plot").toBool()){
        this->setProperty("pause_plot", true);
        ui->action_refresh->setIcon(QIcon(":/resource/work.png"));
        ui->action_refresh->setText(tr("恢复刷新"));
        ui->action_refresh->setIconText(tr("恢复刷新"));
    } else {
        this->setProperty("pause_plot", false);
        ui->action_refresh->setIcon(QIcon(":/resource/pause.png"));
        ui->action_refresh->setText(tr("暂停刷新"));
        ui->action_refresh->setIconText(tr("暂停刷新"));
    }
}

void MainWindow::on_pushButton_gauss_clicked()
{
    this->save();

    PlotWidget* plotWidget = this->findChild<PlotWidget*>("real-PlotWidget");
    plotWidget->slotGauss(ui->spinBox_leftE->value(), ui->spinBox_rightE->value());
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

    qInfo() << tr("恢复出厂设置");
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
    commandHelper->closeDetector();
    commandHelper->closeRelay();
    controlHelper->close();

    event->accept();
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
        ui->pushButton_measure_2_tip->setText("");

        ui->comboBox_range->setEnabled(false);
        ui->comboBox_range_2->setEnabled(false);
        ui->comboBox_range_3->setEnabled(false);
        if (this->property("measur-model").toInt() == mmManual){//手动测量
            ui->pushButton_measure->setText(tr("停止测量"));
            ui->pushButton_measure->setEnabled(true);
            ui->pushButton_refresh->setEnabled(true);

            ui->action_power->setEnabled(false);
            ui->action_detector_connect->setEnabled(false);

            //测量过程中不允许修改能窗幅值
            ui->spinBox_1_leftE->setEnabled(false);
            ui->spinBox_1_rightE->setEnabled(false);
            ui->spinBox_2_leftE->setEnabled(false);
            ui->spinBox_2_rightE->setEnabled(false);

            // 测量中禁用符合分辨时间
            ui->spinBox_timeWidth->setEnabled(false);

            //公共
            ui->pushButton_save->setEnabled(false);
            ui->pushButton_gauss->setEnabled(true);

            //自动测量
            ui->pushButton_measure_2->setEnabled(false);
            ui->pushButton_refresh_2->setEnabled(false);

            //标定测量
            ui->pushButton_measure_3->setEnabled(false);
            ui->pushButton_refresh_3->setEnabled(false);
        } else if (this->property("measur-model").toInt() == mmAuto){//自动测量
            ui->pushButton_measure_2->setText(tr("停止测量"));
            ui->pushButton_measure_2->setEnabled(true);

            //公共
            ui->pushButton_save->setEnabled(false);
            ui->pushButton_gauss->setEnabled(true);

            //手动测量
            ui->pushButton_measure->setEnabled(false);
            ui->pushButton_refresh->setEnabled(true);

            //标定测量
            ui->pushButton_measure_3->setEnabled(false);
        }  else if (this->property("measur-model").toInt() == mmDefine){//标定测量
            ui->pushButton_measure_3->setText(tr("停止测量"));
            ui->pushButton_measure_3->setEnabled(true);

            //公共
            ui->pushButton_save->setEnabled(false);
            ui->pushButton_gauss->setEnabled(true);

            //手动测量
            ui->pushButton_measure->setEnabled(false);
            ui->pushButton_refresh->setEnabled(true);

            //自动测量
            ui->pushButton_measure_2->setEnabled(false);
        }
    } else {
        ui->comboBox_range->setEnabled(true);
        ui->comboBox_range_2->setEnabled(true);
        ui->comboBox_range_3->setEnabled(true);

        ui->pushButton_measure->setText(tr("开始测量"));
        ui->pushButton_measure_2->setText(tr("开始测量"));
        ui->pushButton_measure_3->setText(tr("开始测量"));

        // 手动测量
        ui->spinBox_1_leftE->setEnabled(true);
        ui->spinBox_1_rightE->setEnabled(true);
        ui->spinBox_2_leftE->setEnabled(true);
        ui->spinBox_2_rightE->setEnabled(true);        
        ui->spinBox_timeWidth->setEnabled(true);

        // 自动测量
        ui->spinBox_1_leftE_2->setEnabled(true);
        ui->spinBox_1_rightE_2->setEnabled(true);
        ui->spinBox_2_leftE_2->setEnabled(true);
        ui->spinBox_2_rightE_2->setEnabled(true);
        ui->spinBox_timeWidth_2->setEnabled(true);

        // 标定测量
        ui->spinBox_1_leftE_3->setEnabled(true);
        ui->spinBox_1_rightE_3->setEnabled(true);
        ui->spinBox_2_leftE_3->setEnabled(true);
        ui->spinBox_2_rightE_3->setEnabled(true);
        ui->spinBox_timeWidth_3->setEnabled(true);

        if (this->property("detector_on").toBool()){
//            ui->action_power->setEnabled(true);
//            ui->action_detector_connect->setEnabled(true);
//            ui->action_refresh->setEnabled(false);

            //公共
            ui->pushButton_save->setEnabled(true);

            ui->pushButton_measure->setEnabled(true);
            ui->pushButton_measure_2->setEnabled(true);
            ui->pushButton_measure_3->setEnabled(true);
        } else {
//            ui->action_power->setEnabled(true);
//            ui->action_detector_connect->setEnabled(true);
//            ui->action_refresh->setEnabled(false);

            //公共
            ui->pushButton_save->setEnabled(false);
            ui->pushButton_measure->setEnabled(false);
            ui->pushButton_measure_2->setEnabled(false);
            ui->pushButton_measure_3->setEnabled(false);
        }

        ui->pushButton_gauss->setEnabled(false);
        ui->pushButton_refresh->setEnabled(false);
        ui->pushButton_refresh_2->setEnabled(false);
        ui->pushButton_refresh_3->setEnabled(false);
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
            //量程选取
            ui->comboBox_range->setCurrentIndex(jsonObjM1["range"].toInt());
            //冷却时长
            ui->spinBox_cool_timelength->setValue(jsonObjM1["cool_timelength"].toInt());
            //时间步长
            ui->spinBox_step->setValue(jsonObjM1["step"].toInt());
            //符合分辨时间
            ui->spinBox_timeWidth->setValue(jsonObjM1["timewidth"].toInt());
        }

        //自动
        QJsonObject jsonObjM2;
        if (jsonObj.contains("M2")){
            jsonObjM2 = jsonObj["M2"].toObject();
            //测量时长
            ui->spinBox_timelength_2->setValue(jsonObjM2["timelength"].toInt());
            //量程选取
            ui->comboBox_range_2->setCurrentIndex(jsonObjM2["range"].toInt());
            //时间步长
            ui->spinBox_step_2->setValue(jsonObjM2["step"].toInt());
            //符合分辨时间
            ui->spinBox_timeWidth_2->setValue(jsonObjM2["timewidth"].toInt());
        }

        //标定
        QJsonObject jsonObjM3;
        if (jsonObj.contains("M3")){
            jsonObjM3 = jsonObj["M3"].toObject();
            //测量时长
            ui->spinBox_timelength_3->setValue(jsonObjM3["timelength"].toInt());
            //量程选取
            ui->comboBox_range_3->setCurrentIndex(jsonObjM3["range"].toInt());
            //冷却时长
            ui->spinBox_cool_timelength_3->setValue(jsonObjM3["cool_timelength"].toInt());
            //符合分辨时间
            ui->spinBox_timeWidth_3->setValue(jsonObjM3["timewidth"].toInt());
            //中子产额
            ui->spinBox_neutron_yield->setValue(jsonObjM3["neutron_yield"].toInt());
        }

        //公共
        QJsonObject jsonObjPub;
        if (jsonObj.contains("Public")){
            jsonObjPub = jsonObj["Public"].toObject();
            //高斯拟合
            ui->spinBox_leftE->setValue(jsonObjPub["Gauss_leftE"].toInt());
            ui->spinBox_rightE->setValue(jsonObjPub["Gauss_rightE"].toInt());
            //保存数据
            ui->lineEdit_path->setText(jsonObjPub["path"].toString());
            ui->lineEdit_filename->setText(jsonObjPub["filename"].toString());
        }
    }
}

void MainWindow::save()
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
        if (jsonObj.contains("M1")){
            jsonObjM1 = jsonObj["M1"].toObject();
        } else {
            //jsonObj.insert("M1", jsonObjM1);
        }
        //测量时长
        jsonObjM1["timelength"] = ui->spinBox_timelength->value();
        //量程选取
        jsonObjM1["range"] = ui->comboBox_range->currentIndex();
        //冷却时长
        jsonObjM1["cool_timelength"] = ui->spinBox_cool_timelength->value();
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
        if (!jsonObj.contains("M1")){
            jsonObj.insert("M1", jsonObjM1);
        }

        //自动
        QJsonObject jsonObjM2;
        if (jsonObj.contains("M2")){
            jsonObjM2 = jsonObj["M2"].toObject();
        } else {
            //jsonObj.insert("M2", jsonObjM2);
        }
        //测量时长
        jsonObjM2["timelength"] = ui->spinBox_timelength_2->value();
        //量程选取
        jsonObjM2["range"] = ui->comboBox_range_2->currentIndex();
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
        if (!jsonObj.contains("M2")){
            jsonObj.insert("M2", jsonObjM2);
        }

        //标定
        QJsonObject jsonObjM3;
        if (jsonObj.contains("M3")){
            jsonObjM3 = jsonObj["M3"].toObject();
        } else {
            //jsonObj.insert("M3", jsonObjM3);
        }
        //测量时长
        jsonObjM3["timelength"] = ui->spinBox_timelength_3->value();
        //量程选取
        jsonObjM3["range"] = ui->comboBox_range_3->currentIndex();
        //冷却时长
        jsonObjM3["cool_timelength"] = ui->spinBox_cool_timelength_3->value();
        //符合分辨时间
        jsonObjM3["timeWidth"] = ui->spinBox_timeWidth_3->value();
        //探测器1能窗左侧
        jsonObjM3["Det1_EnWidth_left"] = ui->spinBox_1_leftE_3->value();
        //探测器1能窗右侧
        jsonObjM3["Det1_EnWidth_right"] = ui->spinBox_1_rightE_3->value();
        //探测器2能窗左侧
        jsonObjM3["Det2_EnWidth_left"] = ui->spinBox_2_leftE_3->value();
        //探测器2能窗右侧
        jsonObjM3["Det2_EnWidth_right"] = ui->spinBox_2_rightE_3->value();
        //中子产额
        jsonObjM3["neutron_yield"] = ui->spinBox_neutron_yield->value();
        if (!jsonObj.contains("M3")){
            jsonObj.insert("M3", jsonObjM3);
        }

        //公共
        QJsonObject jsonObjPub;
        if (jsonObj.contains("Public")){
            jsonObjPub = jsonObj["Public"].toObject();
        } else {
            //jsonObj.insert("Public", jsonObjPub);
        }
        //高斯拟合
        jsonObjPub["Gauss_leftE"] = ui->spinBox_leftE->value();
        jsonObjPub["Gauss_rightE"] = ui->spinBox_rightE->value();
        //保存数据
        jsonObjPub["path"] = ui->lineEdit_path->text();
        jsonObjPub["filename"] = ui->lineEdit_filename->text();
        if (!jsonObj.contains("Public")){
            jsonObj.insert("Public", jsonObjPub);
        }

        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QJsonDocument jsonDocNew(jsonObj);
        file.write(jsonDocNew.toJson());
        file.close();
    }
}

void MainWindow::on_pushButton_refresh_2_clicked()
{
    this->save();
}

void MainWindow::on_pushButton_refresh_3_clicked()
{
    this->save();
}

void MainWindow::on_action_line_log_triggered()
{
    if (this->property("ScaleLogarithmicType").toBool()){
        this->setProperty("ScaleLogarithmicType", false);
        PlotWidget* plotWidget = this->findChild<PlotWidget*>("real-PlotWidget");
        plotWidget->slotResetPlot();
        plotWidget->switchDataModel(false);

        ui->action_line_log->setText(tr("对数"));
        ui->action_line_log->setIconText(tr("对数"));
        ui->action_line_log->setIcon(QIcon(":/resource/mathlog.png"));
    } else {
        this->setProperty("ScaleLogarithmicType", true);
        PlotWidget* plotWidget = this->findChild<PlotWidget*>("real-PlotWidget");
        plotWidget->switchDataModel(true);

        ui->action_line_log->setText(tr("线性"));
        ui->action_line_log->setIconText(tr("线性"));
        ui->action_line_log->setIcon(QIcon(":/resource/line.png"));
    }
}

#include "controlwidget.h"
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
    QString arg1 = ui->lineEdit->text();
    if (!isFloat(arg1)){
        ui->lineEdit->clear();
        ui->lineEdit->setPlaceholderText(tr("输入数字无效"));
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
