//DateTimeSelectWidget
#ifndef DATETIMESELECTWIDGET_H
#define DATETIMESELECTWIDGET_H

#include <QWidget>
#include <QDateTime>


namespace Ui {
class DateTimeSelectWidget;
}

class DateTimeSelectWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DateTimeSelectWidget(QWidget *parent = nullptr);
    ~DateTimeSelectWidget();

    void SetSelectNewTime(QString qsCurrentTime);

protected:
    void showEvent(QShowEvent *event);
    virtual void paintEvent(QPaintEvent *);//重载--绘图事件--绘制背景图

private:
    void init_wid_ui();

    void init_signal_and_slot();

    void InitUI();
    void InitDialogData();
    void LoadConnectMsg();


signals:
    void signal_snd_time_update(QString);

private slots:
    void OnBnClickedOK();
    void OnBnClickedCanel();

    void slot_update_cur_time_val();


private:
    Ui::DateTimeSelectWidget *ui;
    // QString curDateTime;


};

#endif

