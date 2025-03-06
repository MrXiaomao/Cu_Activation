#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "commandhelper.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class SpectrumModel;
class DataAnalysisWidget;
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
    void slotPlowWidgetDoubleClickEvent();

    void on_action_power_triggered();

    void on_actionaction_net_connect_triggered();

    void on_action_devices_triggered();

    void on_action_SpectrumModel_triggered();

    void on_action_WaveformModel_triggered();

    void on_action_FPGASetting_triggered();

    void on_action_DataAnalysis_triggered();

private:
    Ui::MainWindow *ui = nullptr;
    SpectrumModel *spectrummodel = nullptr;//能谱模型
    DataAnalysisWidget *dataAnalysisWidget = nullptr;//数据解析
    CommandHelper *commandhelper = nullptr;//网络指令
    bool power_on;//电源打开标识
    bool net_connected;//网络连接标识
};
#endif // MAINWINDOW_H
