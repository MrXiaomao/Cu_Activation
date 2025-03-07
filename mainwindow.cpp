#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "equipmentmanagementform.h"
#include "spectrumModel.h"
#include "dataanalysiswidget.h"
#include "plotwidget.h"
#include "FPGASetting.h"
#include <QFileDialog>
#include <QToolButton>
#include <QTimer>
#include <QScreen>
#include <QButtonGroup>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , power_on(false)
    , net_connected(false)
{
    ui->setupUi(this);

    connect(this, SIGNAL(sigWriteLog(const QString &, log_level)), this, SLOT(slotWriteLog(const QString &, log_level)));
    commandhelper = new CommandHelper(this);
    connect(commandhelper, &CommandHelper::sigPowerStatus, this, [=](bool on){
        power_on = on;
        if (power_on){
            ui->action_power->setIcon(QIcon(":/resource/power-on.png"));
            ui->action_power->setText(tr("关闭电源"));
            ui->action_power->setIconText(tr("关闭电源"));

            ui->statusbar->showMessage(tr("电源已连接"));
            emit sigWriteLog(tr("电源已连接"));
        } else {
            ui->action_power->setIcon(QIcon(":/resource/power-off.png"));
            ui->action_power->setText(tr("打开电源"));
            ui->action_power->setIconText(tr("打开电源"));

            ui->statusbar->showMessage(tr("电源连接失败"));
            emit sigWriteLog(tr("电源连接失败"));
        }
    });
    connect(commandhelper, &CommandHelper::sigRelayStatus, this, [=](bool on1, bool on2){
        QString msg = QString(tr("继电器1：%1，继电器2：%2"))
                .arg(on1 ? tr("开") : tr("关"))
                .arg(on2 ? tr("开") : tr("关"));

        ui->statusbar->showMessage(msg);
        emit sigWriteLog(msg);
    });

    InitMainWindowUi();
    // 创建图表
    initCustomPlot();

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
        QAction *action = ui->lineEdit_20->addAction(QIcon(":/resource/open.png"), QLineEdit::TrailingPosition);
        QToolButton* button = qobject_cast<QToolButton*>(action->associatedWidgets().last());
        button->setCursor(QCursor(Qt::PointingHandCursor));
        connect(button, &QToolButton::pressed, this, [=](){
            QString dir = QFileDialog::getExistingDirectory(this);
            ui->lineEdit_20->setText(dir);
        });
    }

    ui->tabWidget_measure->setCurrentIndex(0);

    // 启用关闭按钮
    ui->tabWidget_client->setTabsClosable(true);
    ui->tabWidget_client->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);//第一个tab取消关闭按钮
    connect(ui->tabWidget_client, &QTabWidget::tabCloseRequested, this, [=](int index){
        ui->tabWidget_client->removeTab(index);
    });

    ui->label_tag->setBuddy(ui->widget_tag);
    ui->label_tag_->setBuddy(ui->widget_tag);
    ui->label_tag->installEventFilter(this);
    ui->label_tag_->installEventFilter(this);
    FPGASetting *fpgaSetting = new FPGASetting(this);
    ui->widget_tag->layout()->addWidget(fpgaSetting);
//    ui->widget_tag->hide();
//    ui->widget_tag->setProperty("toggled", true);

    // 显示计数/能谱
    ui->label_tag_2->setBuddy(ui->widget_tag_2);
    ui->label_tag_2_->setBuddy(ui->widget_tag_2);
    ui->label_tag_2->installEventFilter(this);
    ui->label_tag_2_->installEventFilter(this);
    QButtonGroup *grp = new QButtonGroup(this);
    grp->addButton(ui->radioButton_ref, 0);
    grp->addButton(ui->radioButton_spectrum, 1);
    connect(grp, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [=](int index){
        if (0 == index){
            ui->widget_gauss->hide();
        } else {
            ui->widget_gauss->show();
        }
    });
    ui->widget_gauss->hide();

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
    ui->statusbar->addPermanentWidget(label_systemtime);

    this->slotWriteLog(QObject::tr("系统启动"));

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
}

// 打开电源
void MainWindow::on_action_power_triggered()
{
    if (!power_on){
        commandhelper->openPower("127.0.0.1", 5000);
    } else {
        commandhelper->closePower("127.0.0.1", 5000);
    }
}

// 连接网络
void MainWindow::on_actionaction_net_connect_triggered()
{
    if (net_connected){
        ui->actionaction_net_connect->setIcon(QIcon(":/resource/lianjie.png"));
        ui->actionaction_net_connect->setText(tr("连接外设"));
        net_connected = false;
    } else {
        ui->actionaction_net_connect->setIcon(QIcon(":/resource/quxiaolianjie.png"));
        ui->actionaction_net_connect->setText(tr("断开外设"));
        net_connected = true;
    }
}

void MainWindow::initCustomPlot()
{
    //this->setDockNestingEnabled(true);

    // Det1
    PlotWidget *customPlotWidget_Det1 = new PlotWidget(ui->widget_plot);
    customPlotWidget_Det1->setName(tr("Det1"));
    customPlotWidget_Det1->installEventFilter(this);
    customPlotWidget_Det1->show();

    // Det2
    PlotWidget *customPlotWidget_Det2 = new PlotWidget(ui->widget_plot);
    customPlotWidget_Det2->setName(tr("Det2"));
    customPlotWidget_Det2->installEventFilter(this);
    customPlotWidget_Det2->show();

    // result
    PlotWidget *customPlotWidget_result = new PlotWidget(ui->widget_plot);
    customPlotWidget_result->setName(tr("符合结果"));
    customPlotWidget_result->installEventFilter(this);
    customPlotWidget_result->show();

    ui->widget_plot->layout()->addWidget(customPlotWidget_Det1);
    ui->widget_plot->layout()->addWidget(customPlotWidget_Det2);
    ui->widget_plot->layout()->addWidget(customPlotWidget_result);

    connect(customPlotWidget_Det1, SIGNAL(sigMouseDoubleClickEvent()), this, SLOT(slotPlowWidgetDoubleClickEvent()));
    connect(customPlotWidget_Det2, SIGNAL(sigMouseDoubleClickEvent()), this, SLOT(slotPlowWidgetDoubleClickEvent()));
    connect(customPlotWidget_result, SIGNAL(sigMouseDoubleClickEvent()), this, SLOT(slotPlowWidgetDoubleClickEvent()));
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
    EquipmentManagementForm *equipmentManagementForm = new EquipmentManagementForm(this);
    equipmentManagementForm->setAttribute(Qt::WA_DeleteOnClose, true);
    equipmentManagementForm->setWindowFlags(equipmentManagementForm->windowFlags() |Qt::Dialog);
    equipmentManagementForm->setWindowModality(Qt::ApplicationModal);
    equipmentManagementForm->showNormal();
}

void MainWindow::slotWriteLog(const QString &log, log_level level/* = lower*/)
{
    // 创建一个 QTextCursor
    QTextCursor cursor = ui->textEdit_log->textCursor();
    // 将光标移动到文本末尾
    cursor.movePosition(QTextCursor::End);

    // 先插入时间
    cursor.insertHtml(QString("<span style='color:black;'>%1</span>").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss >> ")));
    // 再插入文本
    if ( lower == level)
        cursor.insertHtml(QString("<span style='color:black;'>%1</span>").arg(log));
    else if ( higher == level)
        cursor.insertHtml(QString("<span style='color:red;'>%1</span>").arg(log));
    else
        cursor.insertHtml(QString("<span style='color:green;'>%1</span>").arg(log));

    // 最后插入换行符
    cursor.insertHtml("<br>");

    // 确保 QTextEdit 显示了光标的新位置
    ui->textEdit_log->setTextCursor(cursor);
}

void MainWindow::on_action_SpectrumModel_triggered()
{
//    if (spectrummodel && spectrummodel->isVisible()){
//        return ;
//    }

//    //能谱测量
//    if (nullptr == spectrummodel){
//        spectrummodel = new SpectrumModel(this);
//        //spectrummodel->setAttribute(Qt::WA_DeleteOnClose, true);
//        //spectrummodel->setWindowModality(Qt::ApplicationModal);
//    }

//    int index = ui->tabWidget_client->addTab(spectrummodel, tr("能谱测量"));
//    ui->tabWidget_client->setCurrentIndex(index);

    QDockWidget *dockWidget = new QDockWidget();
    dockWidget->setAttribute(Qt::WA_DeleteOnClose, true);
    dockWidget->setWindowFlags(dockWidget->windowFlags() |Qt::Dialog);
    dockWidget->setWindowModality(Qt::ApplicationModal);
    dockWidget->setFloating(true);
    dockWidget->setWindowTitle(tr("能谱测量"));
    dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->addDockWidget(Qt::NoDockWidgetArea, dockWidget);

    QWidget *dockWidgetContents = new QWidget(dockWidget);
    dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
    QGridLayout *gridLayout = new QGridLayout(dockWidgetContents);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    gridLayout->setContentsMargins(0, 0, 0, 0);

    SpectrumModel *spectrummodel = new SpectrumModel(dockWidget);
    gridLayout->addWidget(spectrummodel, 0, 0, 1, 1);
    dockWidget->setWidget(dockWidgetContents);
    dockWidget->setGeometry(0,0,spectrummodel->width(), 361/*waveformModel->height()*/ + 12);

    QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();
    int x = (screenRect.width() - 260/*waveformModel->width()*/) / 2;
    int y = (screenRect.height() - 361/*waveformModel->height()*/) / 2;
    dockWidget->move(x, y);
    dockWidget->show();
    dockWidget->activateWindow();
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
    QDockWidget *dockWidget = new QDockWidget();
    dockWidget->setAttribute(Qt::WA_DeleteOnClose, true);
    dockWidget->setWindowFlags(dockWidget->windowFlags() |Qt::Dialog);
    dockWidget->setWindowModality(Qt::ApplicationModal);
    dockWidget->setFloating(true);
    dockWidget->setWindowTitle(tr("波形测量"));
    dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->addDockWidget(Qt::NoDockWidgetArea, dockWidget);

    QWidget *dockWidgetContents = new QWidget(dockWidget);
    dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
    QGridLayout *gridLayout = new QGridLayout(dockWidgetContents);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    gridLayout->setContentsMargins(0, 0, 0, 0);

    WaveformModel *waveformModel = new WaveformModel(dockWidget);
    gridLayout->addWidget(waveformModel, 0, 0, 1, 1);
    dockWidget->setWidget(dockWidgetContents);
    dockWidget->setGeometry(0,0,waveformModel->width(), 410/*waveformModel->height()*/ + 12);

    QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();
    int x = (screenRect.width() - 260/*waveformModel->width()*/) / 2;
    int y = (screenRect.height() - 410/*waveformModel->height()*/) / 2;
    dockWidget->move(x, y);
    dockWidget->show();
    dockWidget->activateWindow();
}

void MainWindow::on_action_detector_conndect_triggered()
{
    if (detector_connected){
        ui->actionaction_net_connect->setText(tr("连接探测器"));
        detector_connected = false;
    } else {
        ui->actionaction_net_connect->setText(tr("断开探测器"));
        detector_connected = true;
    }
}
