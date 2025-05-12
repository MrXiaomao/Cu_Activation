#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include "commandhelper.h"
#include "controlhelper.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class SpectrumModel;
class OfflineDataAnalysisWidget;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void InitMainWindowUi();
    void initCustomPlot();

    void load();
    void saveConfigJson(bool bSafeExitFlag = false);

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    virtual void closeEvent(QCloseEvent *event) override;
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

public slots:
    void slotAppendMsg(const QString &msg, QtMsgType msgType);
    void slotRefreshUi();
    void slotUpdateEnWindow();
    void slotSafeExit();

signals:
    void sigRefreshUi();
    void sigRefreshBoostMsg(const QString &msg);
    void sigAppengMsg(const QString &msg, QtMsgType msgType);

private slots:
    void on_action_power_triggered();

    void on_action_devices_triggered();

    void on_action_SpectrumModel_triggered();

    void on_action_WaveformModel_triggered();

    void on_action_DataAnalysis_triggered();

    void on_action_energycalibration_triggered();

    void on_pushButton_measure_clicked();

    void on_pushButton_measure_2_clicked();

    void on_action_displacement_triggered();

    void on_action_detector_connect_triggered();

    // void on_pushButton_save_clicked(); //测量结果另存为

    void on_pushButton_refresh_clicked();

    void on_action_close_triggered();

    void on_action_refresh_triggered();

    void on_pushButton_gauss_clicked();

    void on_action_config_triggered();

    void on_action_about_triggered();

    void on_action_restore_triggered();

    void on_action_cachepath_triggered();

    void on_action_yieldcalibration_triggered();

    void on_action_log_triggered();

    void on_pushButton_refresh_2_clicked();

    void on_action_line_log_triggered();

    void on_action_Moving_triggered();

    void on_lineEdit_editingFinished();

    void on_action_start_measure_triggered();

    void on_checkBox_gauss_stateChanged(int arg1);

    void on_pushButton_confirm_clicked();

    void on_action_analyze_triggered();

    void on_tabWidget_client_currentChanged(int index);

    void on_action_openfile_triggered();

    void on_action_help_triggered();

    void on_action_viewlog_triggered();

private:
    Ui::MainWindow *ui = nullptr;
    SpectrumModel *spectrummodel = nullptr;//能谱模型
    OfflineDataAnalysisWidget *offlineDataAnalysisWidget = nullptr;//数据查看和分析
    CommandHelper *commandHelper = nullptr;//网络指令
    ControlHelper *controlHelper = nullptr;//网络指令

    qint32 currentDetectorIndex = 0;
    unsigned int multi_CHANNEL; //多道道数
    QDateTime lastRecvDataTime;//探测器上一次接收数据时间
    QDateTime measureStartTime;//探测器测量开始时间
};
#endif // MAINWINDOW_H
