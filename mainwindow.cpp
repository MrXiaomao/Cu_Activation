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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , relay_on(false)
    , detector_connected(false)
{
    ui->setupUi(this);

    emit sigRefreshBoostMsg(tr("更新ui..."));
    connect(this, SIGNAL(sigWriteLog(const QString &, log_level)), this, SLOT(slotWriteLog(const QString &, log_level)));
    commandhelper = CommandHelper::instance();
    connect(commandhelper, &CommandHelper::sigDisplacementFault, this, [=](qint32 index){        
        ui->action_net_connect->setIcon(QIcon(":/resource/lianjie-fault.png"));
        ui->action_net_connect->setText(tr("打开外设"));
        ui->action_net_connect->setIconText(tr("打开外设"));
        net_connected[index] = false;
        QString msg = QString(tr("网络故障，外设%1连接失败！").arg((index + 1)));
        ui->statusbar->showMessage(msg);
        emit sigAppengMsg(msg, QtCriticalMsg);

        slotRefreshUIStatus();
    });
    connect(commandhelper, &CommandHelper::sigDisplacementStatus, this, [=](bool on, qint32 index){
        net_connected[index] = on;
        if (on){
            ui->action_power->setEnabled(true);
            ui->action_net_connect->setIcon(QIcon(":/resource/lianjie-on.png"));
            ui->action_net_connect->setText(tr("关闭外设"));
            ui->action_net_connect->setIconText(tr("关闭外设"));

            //外设连接成功，主动连接电源
            if (net_connected[0] && net_connected[1]){
                QFile file(QApplication::applicationDirPath() + "/config/ip.json");
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    // 读取文件内容
                    QByteArray jsonData = file.readAll();
                    file.close(); //释放资源

                    // 将 JSON 数据解析为 QJsonDocument
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
                    QJsonObject jsonObj = jsonDoc.object();

                    QString ip = "192.168.10.253";
                    qint32 port = 1030;
                    QJsonObject jsonDetector;
                    if (jsonObj.contains("Relay")){
                        jsonDetector = jsonObj["Relay"].toObject();
                        ip = jsonDetector["ip"].toString();
                        port = jsonDetector["port"].toInt();
                    }

                    commandhelper->openRelay(ip, port);
                }
            }
        } else {
            ui->action_net_connect->setIcon(QIcon(":/resource/lianjie.png"));
            ui->action_net_connect->setText(tr("打开外设"));
            ui->action_net_connect->setIconText(tr("打开外设"));
        }

        QString msg = QString(tr("外设%1状态：%2")).arg(index + 1).arg(on ? tr("开") : tr("关"));
        ui->statusbar->showMessage(msg);
        emit sigAppengMsg(msg, QtInfoMsg);

        slotRefreshUIStatus();
    });

    // 禁用开启电源按钮
    ui->action_power->setEnabled(false);
    connect(commandhelper, &CommandHelper::sigRelayFault, this, [=](){
        relay_on = false;
        ui->action_power->setIcon(QIcon(":/resource/lianjie-fault.png"));
        ui->action_power->setText(tr("打开电源"));
        ui->action_power->setIconText(tr("打开电源"));

        detector_connected = false;
        ui->action_detector_connect->setIcon(QIcon(":/resource/conect.png"));
        ui->action_detector_connect->setText(tr("连接探测器"));
        ui->action_detector_connect->setIconText(tr("连接探测器"));

        QString msg = QString(tr("网络故障，电源连接失败！"));
        ui->statusbar->showMessage(msg);
        emit sigAppengMsg(msg, QtCriticalMsg);

        slotRefreshUIStatus();
    });

    connect(commandhelper, &CommandHelper::sigRelayStatus, this, [=](bool on){
        relay_on = on;
        if (relay_on){
            ui->action_detector_connect->setEnabled(true);
            ui->action_power->setIcon(QIcon(":/resource/power-on.png"));
            ui->action_power->setText(tr("关闭电源"));
            ui->action_power->setIconText(tr("关闭电源"));
        } else {
            ui->action_power->setIcon(QIcon(":/resource/power-off.png"));
            ui->action_power->setText(tr("打开电源"));
            ui->action_power->setIconText(tr("打开电源"));

            detector_connected = false;
            ui->action_detector_connect->setIcon(QIcon(":/resource/conect.png"));
            ui->action_detector_connect->setText(tr("连接探测器"));
            ui->action_detector_connect->setIconText(tr("连接探测器"));
        }

        QString msg = QString(tr("电源状态：%1")).arg(on ? tr("开") : tr("关"));
        ui->statusbar->showMessage(msg);
        emit sigAppengMsg(msg, QtInfoMsg);

        slotRefreshUIStatus();
    });

    // 自动测量开始
    connect(commandhelper, &CommandHelper::sigMeasureStart, this, [=](qint8 mode){
        measuring = true;
        if (mode == 0x00){
            ui->pushButton_measure->setText(tr("停止测量"));
            ui->pushButton_measure->setEnabled(true);
        } else if (mode == 0x01){
            ui->pushButton_measure_2->setText(tr("停止测量"));
            ui->pushButton_measure_2->setEnabled(true);
            ui->start_time_text->setDateTime(QDateTime::currentDateTime());
        }
    });

    // 禁用连接探测器按钮
    ui->action_detector_connect->setEnabled(false);
    connect(commandhelper, &CommandHelper::sigDetectorFault, this, [=](){
        detector_connected = false;
        ui->action_detector_connect->setIcon(QIcon(":/resource/conect-fault.png"));
        ui->action_detector_connect->setText(tr("连接探测器"));
        ui->action_detector_connect->setIconText(tr("连接探测器"));
        QString msg = QString(tr("网络故障，探测器连接失败！"));
        ui->statusbar->showMessage(msg);
        emit sigAppengMsg(msg, QtCriticalMsg);

        //手动
        // 禁用开始测量按钮
        ui->pushButton_measure->setEnabled(false);
        // 禁用刷新按钮
        ui->pushButton_refresh->setEnabled(false);

        //自动
        // 禁用等候触发按钮
        ui->pushButton_measure_2->setEnabled(false);
        // 禁用刷新按钮
        ui->pushButton_refresh_2->setEnabled(false);

        //标定
        // 禁用开始测量按钮
        ui->pushButton_measure_3->setEnabled(false);
        // 禁用刷新按钮
        ui->pushButton_refresh_3->setEnabled(false);

        // 禁用高斯拟合按钮
        ui->pushButton_gauss->setEnabled(false);

        slotRefreshUIStatus();
    });

    connect(commandhelper, &CommandHelper::sigDetectorStatus, this, [=](bool on){
        detector_connected = on;
        if (detector_connected){
            ui->action_detector_connect->setIcon(QIcon(":/resource/conect-on.png"));
            ui->action_detector_connect->setText(tr("断开探测器"));
            ui->action_detector_connect->setIconText(tr("断开探测器"));
        } else {
            ui->action_detector_connect->setIcon(QIcon(":/resource/conect.png"));
            ui->action_detector_connect->setText(tr("连接探测器"));
            ui->action_detector_connect->setIconText(tr("连接探测器"));
        }

        //符合测量
        ui->action_partical->setEnabled(detector_connected);
        //波形测量
        ui->action_WaveformModel->setEnabled(detector_connected);
        //能谱测量
        ui->action_SpectrumModel->setEnabled(detector_connected);

        //手动
        // 禁用开始测量按钮
        ui->pushButton_measure->setEnabled(detector_connected);
        // 禁用刷新按钮
        ui->pushButton_refresh->setEnabled(detector_connected);

        //自动
        // 禁用等候触发按钮
        ui->pushButton_measure_2->setEnabled(detector_connected);
        // 禁用刷新按钮
        ui->pushButton_refresh_2->setEnabled(detector_connected);

        //标定
        // 禁用开始测量按钮
        ui->pushButton_measure_3->setEnabled(detector_connected);
        // 禁用刷新按钮
        ui->pushButton_refresh_3->setEnabled(detector_connected);

        // 禁用高斯拟合按钮
        //ui->pushButton_gauss->setEnabled(detector_connected);

        QString msg = QString(tr("探测器状态：%1")).arg(on ? tr("开") : tr("关"));
        ui->statusbar->showMessage(msg);
        emit sigAppengMsg(msg, QtInfoMsg);
        slotRefreshUIStatus();
    });

    qRegisterMetaType<PariticalCountFrame>("PariticalCountFrame");
    qRegisterMetaType<std::vector<TimeCountRate>>("std::vector<TimeCountRate>");
    connect(commandhelper, &CommandHelper::sigRefreshCountData, this, [=](PariticalCountFrame frame){
        if (!pause_plot){
            PlotWidget* plotWidget = this->findChild<PlotWidget*>(QString("real-Detector-%1").arg(frame.channel+1));
            plotWidget->slotUpdateCountData(frame);
        }
    });
    qRegisterMetaType<PariticalSpectrumFrame>("PariticalSpectrumFrame");
    connect(commandhelper, &CommandHelper::sigRefreshSpectrum, this, [=](PariticalSpectrumFrame frame){
        if (!pause_plot){
            PlotWidget* plotWidget = this->findChild<PlotWidget*>(QString("real-Detector-%1").arg(frame.channel+1));
            plotWidget->slotUpdateSpectrumData(frame);
        }
    });

    //手动
    // 禁用开始测量按钮
    ui->pushButton_measure->setEnabled(false);
    // 禁用刷新按钮
    ui->pushButton_refresh->setEnabled(false);

    //自动
    // 禁用等候触发按钮
    ui->pushButton_measure_2->setEnabled(false);
    // 禁用刷新按钮
    ui->pushButton_refresh_2->setEnabled(false);

    //标定
    // 禁用开始测量按钮
    ui->pushButton_measure_3->setEnabled(false);
    // 禁用刷新按钮
    ui->pushButton_refresh_3->setEnabled(false);

    // 禁用高斯拟合按钮
    ui->pushButton_gauss->setEnabled(false);

    // 创建图表
    emit sigRefreshBoostMsg(tr("创建图形库..."));
    initCustomPlot();

    emit sigRefreshBoostMsg(tr("读取参数设置ui..."));
    InitMainWindowUi();

    QTimer::singleShot(500, this, [=](){
       this->showMaximized();
    });
}

MainWindow::~MainWindow()
{
    if (commandhelper)
    {
        delete commandhelper;
        commandhelper = nullptr;
    }

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
    ui->start_time_text->setDateTime(QDateTime::currentDateTime());
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
    } else {
        ui->lineEdit_path->setText("./");
        ui->lineEdit_filename->setText("test.dat");
    }

    // 手动测量-标定文件
    {
        QAction *action = ui->lineEdit_9->addAction(QIcon(":/resource/open.png"), QLineEdit::TrailingPosition);
        QToolButton* button = qobject_cast<QToolButton*>(action->associatedWidgets().last());
        button->setCursor(QCursor(Qt::PointingHandCursor));
        connect(button, &QToolButton::pressed, this, [=](){
            QString lastDir = QApplication::applicationFilePath();
            QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件"), lastDir, tr("所有文件（*.*）"));
            if (!fileName.isEmpty()){
                lastDir = fileName;
                ui->lineEdit_9->setText(fileName);
            }
        });
    }
    // 自动测量-标定文件
    {
        QAction *action = ui->lineEdit_18->addAction(QIcon(":/resource/open.png"), QLineEdit::TrailingPosition);
        QToolButton* button = qobject_cast<QToolButton*>(action->associatedWidgets().last());
        button->setCursor(QCursor(Qt::PointingHandCursor));
        connect(button, &QToolButton::pressed, this, [=](){
            QString dir = QFileDialog::getOpenFileName(this);
            ui->lineEdit_18->setText(dir);
        });
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

    // 显示计数/能谱
    ui->label_tag_2->setBuddy(ui->widget_tag_2);
    ui->label_tag_2->installEventFilter(this);
    ui->label_tag_2->setStyleSheet("");
    QButtonGroup *grp = new QButtonGroup(this);
    grp->addButton(ui->radioButton_ref, 0);
    grp->addButton(ui->radioButton_spectrum, 1);
    connect(grp, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [=](int index){
        for (int i=0; i<2; ++i){
            PlotWidget* plotWidget = this->findChild<PlotWidget*>(QString("real-Detector-%1").arg(i+1));
            plotWidget->slotResetPlot();
        }

        if (0 == index){
            ui->widget_gauss->hide();

            PlotWidget* plotWidget = this->findChild<PlotWidget*>(tr("real-Result"));
            plotWidget->show();
            ui->widget_result->show();
        } else {
            ui->widget_gauss->show();
            PlotWidget* plotWidget = this->findChild<PlotWidget*>(tr("real-Result"));
            plotWidget->hide();
            ui->widget_result->hide();
        }

        commandhelper->updateShowModel(index == 0);
    });
    emit grp->buttonClicked(1);

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
    label_Connected->setText(tr("状态：未连接"));

    /*设置任务栏信息*/
    QLabel *label_systemtime = new QLabel(ui->statusbar);
    label_systemtime->setObjectName("label_systemtime");
    label_systemtime->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ui->statusbar->setContentsMargins(5, 0, 5, 0);
    ui->statusbar->addWidget(label_Idle);
    ui->statusbar->addWidget(label_Connected);
    ui->statusbar->addWidget(new QLabel(ui->statusbar), 1);
    ui->statusbar->addWidget(nullptr, 1);
    ui->statusbar->addPermanentWidget(label_systemtime);    

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=](){
        static QDateTime startTime = QDateTime::currentDateTime();
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
    timer->start(500);

    QTimer::singleShot(500, this, [=](){
       this->showMaximized();
    });
    ui->pushButton_save->setEnabled(false);

    this->slotAppendMsg(QObject::tr("系统启动"), QtInfoMsg);
}

#include <QSplitter>
void MainWindow::initCustomPlot()
{
    //this->setDockNestingEnabled(true);

    QSplitter *spMainWindow = new QSplitter(Qt::Vertical, nullptr);

    // Det-1
    {
        //能谱
        //计数
        //拟合曲线
        PlotWidget *customPlotWidget = new PlotWidget(spMainWindow);
        customPlotWidget->setObjectName(tr("real-Detector-1"));
        customPlotWidget->installEventFilter(this);
        customPlotWidget->setWindowTitle(tr("Det 1"));
        customPlotWidget->initMultiCustomPlot();
        //customPlotWidget->initDetailWidget();
        customPlotWidget->show();

        connect(customPlotWidget, &PlotWidget::sigUpdateMeanValues, this, [=](unsigned int channel, unsigned int minMean, unsigned int maxMean){
            currentDetectorIndex = channel;
            if (channel == 0){
                ui->spinBox_1_leftE->setValue(minMean);
                ui->spinBox_1_rightE->setValue(maxMean);
            }

           ui->spinBox_leftE->setValue(minMean);
           ui->spinBox_rightE->setValue(maxMean);
        });
        spMainWindow->addWidget(customPlotWidget);
        connect(customPlotWidget, SIGNAL(sigMouseDoubleClickEvent()), this, SLOT(slotPlowWidgetDoubleClickEvent()));
    }

    // Det
    {
        //能谱
        //计数
        //拟合曲线
        PlotWidget *customPlotWidget = new PlotWidget(spMainWindow);
        customPlotWidget->setObjectName(tr("real-Detector-2"));
        customPlotWidget->installEventFilter(this);
        customPlotWidget->setWindowTitle(tr("Det 2"));
        customPlotWidget->initMultiCustomPlot();
        //customPlotWidget->initDetailWidget();
        customPlotWidget->show();

        connect(customPlotWidget, &PlotWidget::sigUpdateMeanValues, this, [=](unsigned int channel, unsigned int minMean, unsigned int maxMean){
            currentDetectorIndex = channel;
            if (channel == 0){
                ui->spinBox_2_leftE->setValue(minMean);
                ui->spinBox_2_rightE->setValue(maxMean);
            }

            ui->spinBox_leftE->setValue(minMean);
            ui->spinBox_rightE->setValue(maxMean);
        });
        spMainWindow->addWidget(customPlotWidget);
        connect(customPlotWidget, SIGNAL(sigMouseDoubleClickEvent()), this, SLOT(slotPlowWidgetDoubleClickEvent()));
    }

    // result
    {
        PlotWidget *customPlotWidget_result = new PlotWidget(spMainWindow);
        customPlotWidget_result->setObjectName(tr("real-Result"));
        customPlotWidget_result->installEventFilter(this);
        customPlotWidget_result->setWindowTitle(tr("符合结果"));
        customPlotWidget_result->initCustomPlot();
        customPlotWidget_result->show();
        //ui->widget_plot->layout()->addWidget(customPlotWidget_result);
        spMainWindow->addWidget(customPlotWidget_result);
        connect(customPlotWidget_result, SIGNAL(sigMouseDoubleClickEvent()), this, SLOT(slotPlowWidgetDoubleClickEvent()));
    }

    spMainWindow->setStretchFactor(0, 3);
    spMainWindow->setStretchFactor(1, 3);
    spMainWindow->setStretchFactor(2, 2);
    ui->widget_plot->layout()->addWidget(spMainWindow);
}

void MainWindow::slotPlowWidgetDoubleClickEvent()
{
    PlotWidget* plotwidget = qobject_cast<PlotWidget*>(sender());
    bool isFullRect = false;
    if (plotwidget->property("isFullRect").isValid())
        isFullRect = plotwidget->property("isFullRect").toBool();

    isFullRect = !isFullRect;
    plotwidget->setProperty("isFullRect", isFullRect);
    QList<PlotWidget*> plotWidgets = this->findChildren<PlotWidget*>();
    for (auto a : plotWidgets){
        bool _isFullRect = false;
        if (a->property("isFullRect").isValid()){
            _isFullRect = a->property("isFullRect").toBool();
        }

        if (isFullRect){
            if (a != plotwidget)
                a->hide();
            else
                a->show();
        } else {
            a->show();
        }
    };

    plotwidget->show();
}

void MainWindow::on_action_devices_triggered()
{
    EquipmentManagementForm *w = new EquipmentManagementForm(this);
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    w->setWindowFlags(w->windowFlags() |Qt::Dialog);
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
    cursor.insertHtml(QString("<span style='color:black;'>%1</span>").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss >> ")));
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
void MainWindow::on_action_net_connect_triggered()
{
    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::information(this, tr("提示"), tr("请先配置远程设备信息！"));
        return;
    }

    // 读取文件内容
    QByteArray jsonData = file.readAll();
    file.close(); //释放资源

    // 将 JSON 数据解析为 QJsonDocument
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObj = jsonDoc.object();

    QString ip = "192.168.10.253";
    qint32 port = 1030;
    if (jsonObj.contains("Displacement")){
        QJsonArray jsonDisplacements = jsonObj["Displacement"].toArray();
        QJsonObject jsonDisplacement;
        for (int i=0; i<jsonDisplacements.size(); ++i){
            jsonDisplacement = jsonDisplacements.at(0).toObject();
            ip = jsonDisplacement["ip"].toString();
            port = jsonDisplacement["port"].toInt();

            //位移平台
            if (!net_connected[i]){
                commandhelper->openDisplacement(ip, port, i);
            } else {
                commandhelper->closeRelay();
                commandhelper->closeDisplacement(i);
            }
        }
    }
}

// 打开电源
void MainWindow::on_action_power_triggered()
{
    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::information(this, tr("提示"), tr("请先配置远程设备信息！"));
        return;
    }

    // 读取文件内容
    QByteArray jsonData = file.readAll();
    file.close(); //释放资源

    // 将 JSON 数据解析为 QJsonDocument
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObj = jsonDoc.object();

    QString ip = "192.168.10.253";
    qint32 port = 1030;
    QJsonObject jsonDetector;
    if (jsonObj.contains("Relay")){
        jsonDetector = jsonObj["Relay"].toObject();
        ip = jsonDetector["ip"].toString();
        port = jsonDetector["port"].toInt();
    }

    if (!relay_on){
        commandhelper->openRelay(ip, port);
    } else {
        commandhelper->closeRelay();
    }
}


// 打开探测器
void MainWindow::on_action_detector_connect_triggered()
{
    if (!detector_connected){
        QFile file(QApplication::applicationDirPath() + "/config/ip.json");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::information(this, tr("提示"), tr("请先配置远程设备信息！"));
            return;
        }

        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        // 将 JSON 数据解析为 QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        QString ip = "192.168.10.3";
        qint32 port = 5000;
        QJsonObject jsonDetector;
        if (jsonObj.contains("Detector")){
            jsonDetector = jsonObj["Detector"].toObject();
            ip = jsonDetector["ip"].toString();
            port = jsonDetector["port"].toInt();
        }

        commandhelper->openDetector(ip, port);
    } else {
        measuring = false;
        ui->pushButton_measure->setText(tr("开始测量"));
        commandhelper->slotStopManualMeasure();
        commandhelper->closeDetector();
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
    if (!measuring){
        //手动测量
        DetectorParameter detectorParameter;
        detectorParameter.triggerThold1 = 0x81;
        detectorParameter.triggerThold2 = 0x81;
        detectorParameter.waveformPolarity = 0x00;
        detectorParameter.dieTimeLength = 0x05;
        detectorParameter.gain = 0x00;
        detectorParameter.transferModel = 0x05;// 0x00-能谱 0x03-波形 0x05-符合模式
        detectorParameter.measureModel = 0x00;

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
        ui->actionaction_line_log->setEnabled(true);
        //ui->pushButton_measure->setText(tr("停止测量"));
        ui->pushButton_measure->setEnabled(false);
        for (int i=0; i<2; ++i){
            PlotWidget* plotWidget = this->findChild<PlotWidget*>(QString("real-Detector-%1").arg(i+1));
            plotWidget->slotResetPlot();
        }

        ui->pushButton_measure_2->setEnabled(false);
        int stepT = ui->spinBox_step->value();
        int leftE[2] = {ui->spinBox_1_leftE->value(), ui->spinBox_2_leftE->value()};
        int rightE[2] = {ui->spinBox_1_rightE->value(), ui->spinBox_2_rightE->value()};
        commandhelper->updateParamter(stepT, leftE, rightE);
        commandhelper->slotStartManualMeasure(detectorParameter);

        QTimer::singleShot(3000, this, [=](){
            //指定时间未收到开始测量指令，则按钮恢复初始状态
            if (!measuring){
                ui->pushButton_measure->setEnabled(true);
            }
        });
    } else {
        measuring = false;
        ui->pushButton_save->setEnabled(true);
        ui->pushButton_gauss->setEnabled(false);
        ui->pushButton_measure->setText(tr("开始测量"));
        ui->action_refresh->setEnabled(false);
        ui->actionaction_line_log->setEnabled(false);
        ui->pushButton_measure_2->setEnabled(true);
        commandhelper->slotStopManualMeasure();
    }
}

void MainWindow::on_pushButton_measure_2_clicked()
{
    //自动测量
    if (!measuring){
        //手动测量
        DetectorParameter detectorParameter;
        detectorParameter.triggerThold1 = 0x81;
        detectorParameter.triggerThold2 = 0x81;
        detectorParameter.waveformPolarity = 0x00;
        detectorParameter.dieTimeLength = 0x05;
        detectorParameter.gain = 0x00;
        detectorParameter.transferModel = 0x05;// 0x00-能谱 0x03-波形 0x05-符合模式
        detectorParameter.measureModel = 0x01;

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
        ui->actionaction_line_log->setEnabled(true);
        ui->pushButton_measure->setEnabled(false);
        ui->pushButton_measure_2->setText(tr("等待触发"));
        ui->pushButton_measure_2->setEnabled(false);
        ui->start_time_text->setDateTime(QDateTime(QDate(0,0,0), QTime(0, 0, 0)));
        for (int i=0; i<2; ++i){
            PlotWidget* plotWidget = this->findChild<PlotWidget*>(QString("real-Detector-%1").arg(i+1));
            plotWidget->slotResetPlot();
        }

        int stepT = ui->spinBox_step_2->value();
        int leftE[2] = {ui->spinBox_1_leftE->value(), ui->spinBox_2_leftE_2->value()};
        int rightE[2] = {ui->spinBox_1_rightE_2->value(), ui->spinBox_2_rightE_2->value()};
        commandhelper->updateParamter(stepT, leftE, rightE);
        commandhelper->slotStartAutoMeasure(detectorParameter);

        QTimer::singleShot(60000, this, [=](){
            //指定时间未收到开始测量指令，则按钮恢复初始状态
            if (!measuring){
                ui->pushButton_measure_2->setEnabled(true);
            }
        });
    } else {
        measuring = false;
        ui->pushButton_save->setEnabled(true);
        ui->pushButton_gauss->setEnabled(false);
        ui->pushButton_measure_2->setText(tr("开始测量"));
        ui->action_refresh->setEnabled(false);
        ui->actionaction_line_log->setEnabled(false);
        ui->pushButton_measure->setEnabled(true);
        commandhelper->slotStopAutoMeasure();
    }
}

void MainWindow::on_pushButton_measure_3_clicked()
{
    //标定测量
}

void MainWindow::on_pushButton_save_clicked()
{
    //存储路径
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

        commandhelper->saveFileName(path + "/" + filename);
    }       
}

void MainWindow::on_pushButton_refresh_clicked()
{
    int stepT = ui->spinBox_step->value();
    int leftE[2] = {ui->spinBox_1_leftE->value(), ui->spinBox_2_leftE->value()};
    int rightE[2] = {ui->spinBox_1_rightE->value(), ui->spinBox_2_rightE->value()};
    commandhelper->updateParamter(stepT, leftE, rightE);
}

void MainWindow::on_actionaction_close_triggered()
{
    this->close();
}

void MainWindow::on_action_refresh_triggered()
{
    pause_plot = !pause_plot;
    if (pause_plot){
        ui->action_refresh->setIcon(QIcon(":/resource/work.png"));
        ui->action_refresh->setText(tr("恢复刷新"));
        ui->action_refresh->setIconText(tr("恢复刷新"));
    } else {
        ui->action_refresh->setIcon(QIcon(":/resource/pause.png"));
        ui->action_refresh->setText(tr("暂停刷新"));
        ui->action_refresh->setIconText(tr("暂停刷新"));
    }
}

void MainWindow::on_pushButton_gauss_clicked()
{
    PlotWidget* plotWidget = this->findChild<PlotWidget*>(QString("real-Detector-%1").arg(currentDetectorIndex+1));
    plotWidget->slotGauss(ui->spinBox_leftE->value(), ui->spinBox_rightE->value());
}

void MainWindow::on_action_config_triggered()
{
    FPGASetting *w = new FPGASetting(this);
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    w->setWindowFlags(Qt::WindowMinMaxButtonsHint|Qt::WindowCloseButtonHint|Qt::Dialog);
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

void MainWindow::on_action_restore_triggered()
{
    if (QMessageBox::warning(this, tr("提示"), tr("是否恢复出厂设置？"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
        return ;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (measuring){
        QMessageBox::warning(this, tr("提示"), tr("测量过程中，禁止退出程序！"), QMessageBox::Ok, QMessageBox::Ok);
        event->ignore();
        return ;
    }

    event->accept();
}

void MainWindow::slotRefreshUIStatus()
{
    ui->action_detector_connect->setEnabled(true);
    ui->action_power->setEnabled(net_connected[0] && net_connected[1]);
    ui->action_detector_connect->setEnabled(relay_on);

    ui->action_config->setEnabled(detector_connected);
    ui->action_restore->setEnabled(detector_connected);

    ui->action_refresh->setEnabled(measuring);
    ui->actionaction_line_log->setEnabled(measuring);
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
