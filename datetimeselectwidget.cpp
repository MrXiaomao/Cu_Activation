#include "datetimeselectwidget.h"
#include "ui_datetimeselectwidget.h"
// #include "rollingtimewidget.h"
#include "QSingleSelectTimeWidget.h"
#include <QPainter>

DateTimeSelectWidget::DateTimeSelectWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DateTimeSelectWidget)
{
    ui->setupUi(this);

    init_wid_ui();

    init_signal_and_slot();
}

DateTimeSelectWidget::~DateTimeSelectWidget()
{
    delete ui;
}

void DateTimeSelectWidget::SetSelectNewTime(QString qsCurrentTime)
{
    QStringList list = qsCurrentTime.split(" ");
    if(list.size() != 2)
        return;

    QString date = list[0];
    QString time = list[1];
    QStringList datelist = date.split("-");
    QStringList timelist = time.split(":");
    if(datelist.size()!=3 || timelist.size()!=3)
        return;

    ui->label_curDate->setText(qsCurrentTime);
}

void DateTimeSelectWidget::showEvent(QShowEvent *event)
{
    QString strDate = ui->label_curDate->text();
    QStringList list = strDate.split(" ");
    if(list.size() != 2)
        return;

    QString date = list[0];
    QString time = list[1];
    QStringList datelist = date.split("-");
    QStringList timelist = time.split(":");
    if(datelist.size()!=3 || timelist.size()!=3)
        return;

    ui->calendarWidget->setSelectedDate(QDate().fromString(date, "yyyy-MM-dd"));
    ui->wid_hour_select->SetCurrentShowTime(QSingleSelectTimeWidget::TimeMode_Hour, QString(timelist[0]).toInt());
    ui->wid_minute_select->SetCurrentShowTime(QSingleSelectTimeWidget::TimeMode_Minute, QString(timelist[1]).toInt());
    ui->wid_second_select->SetCurrentShowTime(QSingleSelectTimeWidget::TimeMode_Second, QString(timelist[2]).toInt());

    QWidget::showEvent(event);
}

void DateTimeSelectWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(this->rect(), QColor(255, 255, 255));
}

void DateTimeSelectWidget::init_wid_ui()
{
    // this->setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint);

    // //设置时分秒的范围
    // ui->wid_hour_select->setTimeRange(0,23);
    // ui->wid_minute_select->setTimeRange(0,59);
    // ui->wid_second_select->setTimeRange(0,59);

    //<button>确定
    QString qsOkStyle= "QPushButton{outline:none;font-family:Microsoft YaHei UI; font-size:16px;background-color:#FFFFFF;}"
                        "QPushButton{color: #1576fc;} QPushButton:hover{color: #67a7ff;}"
                        "QPushButton:pressed{color: #1d71e5;} QPushButton:disabled{color: #CCCCCC;}";
    // ui->btn_ok->setGeometry(110, 245, 100, 50);
    ui->btn_ok->setStyleSheet(qsOkStyle);
    ui->btn_ok->setText(QStringLiteral("确定"));

    //<button>取消
    QString qsCancelStyle = "QPushButton{outline:none;font-family:Microsoft YaHei UI; font-size:16px;background-color:#FFFFFF;}"
                            "QPushButton{color: #999999;} QPushButton:hover{color: #808080;} QPushButton:pressed{color: #696969;}";
    // ui->btn_cancel->setGeometry(10, 245, 100, 50);
    ui->btn_cancel->setStyleSheet(qsCancelStyle);
    ui->btn_cancel->setText(QStringLiteral("取消"));
}

void DateTimeSelectWidget::init_signal_and_slot()
{
    // 点击间后更新
    connect(ui->wid_hour_select, SIGNAL(signal_update_cur_time_val()), this, SLOT(slot_update_cur_time_val()));
    connect(ui->wid_minute_select, SIGNAL(signal_update_cur_time_val()), this, SLOT(slot_update_cur_time_val()));
    connect(ui->wid_second_select, SIGNAL(signal_update_cur_time_val()), this, SLOT(slot_update_cur_time_val()));

    //日历点击之后，更新
    connect(ui->calendarWidget, &QCalendarWidget::selectionChanged, this, &DateTimeSelectWidget::slot_update_cur_time_val);
    //<button>确定
    connect(ui->btn_ok, SIGNAL(clicked()), this, SLOT(OnBnClickedOK()));
    //<button>取消
    connect(ui->btn_cancel, SIGNAL(clicked()), this, SLOT(OnBnClickedCanel()));
}

void DateTimeSelectWidget::OnBnClickedOK()
{
    QDate curDate = ui->calendarWidget->selectedDate();
    if(curDate.isValid()){
        QString curDateTime = curDate.toString("yyyy-MM-dd");
        //组装数据进行发送
        int nHour = ui->wid_hour_select->GetCurrentContent();
        int nMinute = ui->wid_minute_select->GetCurrentContent();
        int nSecond = ui->wid_second_select->GetCurrentContent();
        QString qsGroup = QString::number(nHour).rightJustified(2, '0') \
                          + QStringLiteral(":") + QString::number(nMinute).rightJustified(2, '0') \
                          + QStringLiteral(":") + QString::number(nSecond).rightJustified(2, '0');
        QString dateTime = curDateTime + " " + qsGroup;
        emit signal_snd_time_update(dateTime);
        this->close();
    }
}

void DateTimeSelectWidget::slot_update_cur_time_val()
{
    QDate curDate = ui->calendarWidget->selectedDate();
    if(curDate.isValid()){
        QString curDateTime = curDate.toString("yyyy-MM-dd");
        //组装数据进行发送
        int nHour = ui->wid_hour_select->GetCurrentContent();
        int nMinute = ui->wid_minute_select->GetCurrentContent();
        int nSecond = ui->wid_second_select->GetCurrentContent();
        QString qsGroup = QString::number(nHour).rightJustified(2, '0') \
                          + QStringLiteral(":") + QString::number(nMinute).rightJustified(2, '0') \
                          + QStringLiteral(":") + QString::number(nSecond).rightJustified(2, '0');
        QString dateTime = curDateTime + " " + qsGroup;
        ui->label_curDate->setText(dateTime);
    }
    // QStringList list = ui->lab_hour_minute_second->text().split(":");
    // if(list.size() != 3)
    //     return;

    // RollingTimeWidget *wid = qobject_cast<RollingTimeWidget *>(sender());
    // if(wid == ui->wid_hour_select){
    //     ui->lab_hour_minute_second->setText(QString("%1:%2:%3").arg(val).arg(list[1]).arg(list[2]));
    // }else if(wid == ui->wid_minute_select){
    //     ui->lab_hour_minute_second->setText(QString("%1:%2:%3").arg(list[0]).arg(val).arg(list[2]));
    // }else if(wid == ui->wid_second_select){
    //     ui->lab_hour_minute_second->setText(QString("%1:%2:%3").arg(list[0]).arg(list[1]).arg(val));
    // }
}


void DateTimeSelectWidget::OnBnClickedCanel()
{
    this->close();
}

