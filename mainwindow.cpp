#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "datetimeselectwidget.h"
//#include "qcustomplot.h"
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

void MainWindow::InitMainWindowUi()
{
    // 获取当前时间
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString currentTime = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");
    ui->start_time_text->setText(currentTime);

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
}

void MainWindow::on_btn_select_start_time_clicked()
{
    /*显示继承QWidget对象窗口*/
    if (nullptr == m_pTimeWidget){
        m_pTimeWidget = new DateTimeSelectWidget(this);
        m_pTimeWidget->setAttribute(Qt::WA_DeleteOnClose, true);
        m_pTimeWidget->setWindowFlags(m_pTimeWidget->windowFlags() |Qt::Dialog);
        m_pTimeWidget->setWindowModality(Qt::ApplicationModal);
        m_pTimeWidget->setWindowModality(Qt::WindowModal);
        connect(m_pTimeWidget, &DateTimeSelectWidget::signal_snd_time_update, this, &MainWindow::MsgReceived_CaseTime_NewTime);
    }

    QPoint position = ui->btn_select_start_time->pos();
    int height = ui->btn_select_start_time->height();
    m_pTimeWidget->SetSelectNewTime(ui->start_time_text->text());
    m_pTimeWidget->move(position.x(),position.y()+height);
    m_pTimeWidget->show();
}

void MainWindow::MsgReceived_CaseTime_NewTime(QString qsTime)
{
    ui->start_time_text->setText(qsTime);
}

// 打开电源
void MainWindow::on_action_power_triggered()
{
    if (power_on){
        ui->action_power->setIcon(QIcon(":/resource/power-off.png"));
        ui->action_power->setText(tr("打开电源"));
        power_on = false;
    } else {
        ui->action_power->setIcon(QIcon(":/resource/power-on.png"));
        ui->action_power->setText(tr("关闭电源"));
        power_on = true;
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
