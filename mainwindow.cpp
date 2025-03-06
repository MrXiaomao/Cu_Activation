#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "datetimeselectwidget.h"
#include "equipmentmanagementform.h"
#include "plotwidget.h"
#include <QFileDialog>
#include <QToolButton>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_pTimeWidget(nullptr)
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
    if (m_pTimeWidget)
    {
        delete m_pTimeWidget;
        m_pTimeWidget = nullptr;
    }

    delete ui;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::InitMainWindowUi()
{
    // 获取当前时间
    ui->start_time_text->setDateTime(QDateTime::currentDateTime());
    ui->stop_time_text->setDateTime(QDateTime::currentDateTime());
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

void MainWindow::on_btn_select_start_time_clicked()
{
//    /*显示继承QWidget对象窗口*/
//    if (nullptr == m_pTimeWidget){
//        m_pTimeWidget = new DateTimeSelectWidget(this);
//        //m_pTimeWidget->setAttribute(Qt::WA_DeleteOnClose, true);
//        m_pTimeWidget->setWindowFlags(m_pTimeWidget->windowFlags() |Qt::Dialog);
//        m_pTimeWidget->setWindowModality(Qt::ApplicationModal);
//        m_pTimeWidget->setWindowModality(Qt::WindowModal);
//        connect(m_pTimeWidget, &DateTimeSelectWidget::signal_snd_time_update, this, &MainWindow::MsgReceived_CaseTime_NewTime);
//    }

//    QPoint position = ui->btn_select_start_time->pos();
//    int height = ui->btn_select_start_time->height();
//    m_pTimeWidget->SetSelectNewTime(ui->start_time_text->text());
//    m_pTimeWidget->move(position.x(),position.y()+height);
//    m_pTimeWidget->show();
}

void MainWindow::MsgReceived_CaseTime_NewTime(QString qsTime)
{
    //ui->start_time_text->setText(qsTime);
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
        ui->actionaction_net_connect->setIcon(QIcon(":/resource/quxiaolianjie.png"));
        ui->actionaction_net_connect->setText(tr("打开网络"));
        net_connected = false;
    } else {
        ui->actionaction_net_connect->setIcon(QIcon(":/resource/lianjie.png"));
        ui->actionaction_net_connect->setText(tr("断开网络"));
        net_connected = true;
    }
}

void MainWindow::initCustomPlot()
{
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
