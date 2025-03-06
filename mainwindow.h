#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "datetimeselectwidget.h" //日历时间
#include "commandhelper.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QCustomPlot;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void InitMainWindowUi();
    void initCustomPlot();

    // 0-黑色 1-红色 2-绿色
    enum log_level{lower, higher};

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void slotWriteLog(const QString &log, log_level level = lower);

signals:
    void sigWriteLog(const QString &log, log_level level = lower);

private slots:
    void on_btn_select_start_time_clicked();
    void MsgReceived_CaseTime_NewTime(QString qsTime); //更新测量时间
    void slotPlowWidgetDoubleClickEvent();

    void on_action_power_triggered();

    void on_actionaction_net_connect_triggered();

    void on_action_devices_triggered();

private:
    Ui::MainWindow *ui;
    DateTimeSelectWidget* m_pTimeWidget; //日期 时间选择窗口
    CommandHelper *commandhelper;//网络指令
    bool power_on;//电源打开标识
    bool net_connected;//网络连接标识
};
#endif // MAINWINDOW_H
