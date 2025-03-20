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

    void load();
    void save();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    virtual void closeEvent(QCloseEvent *event) override;

public slots:
    void slotAppendMsg(const QString &msg, QtMsgType msgType);
    void slotRefreshUi();

signals:
    void sigRefreshUi();
    void sigRefreshBoostMsg(const QString &msg);
    void sigAppengMsg(const QString &msg, QtMsgType msgType);

private slots:
    void slotPlowWidgetDoubleClickEvent();

    void on_action_power_triggered();

    void on_action_devices_triggered();

    void on_action_SpectrumModel_triggered();

    void on_action_WaveformModel_triggered();

    void on_action_DataAnalysis_triggered();

    void on_action_energycalibration_triggered();

    void on_pushButton_measure_clicked();

    void on_pushButton_measure_2_clicked();

    void on_pushButton_measure_3_clicked();

    void on_action_displacement_triggered();

    void on_action_detector_connect_triggered();

    void on_pushButton_save_clicked();

    void on_pushButton_refresh_clicked();

    void on_action_close_triggered();

    void on_action_refresh_triggered();

    void on_pushButton_gauss_clicked();

    void on_action_config_triggered();

    void on_action_about_triggered();

    void on_action_restore_triggered();

    void on_action_cachepath_triggered();

    void on_action_log_triggered();

    void on_pushButton_refresh_2_clicked();

    void on_pushButton_refresh_3_clicked();

    void on_action_line_log_triggered();

private:
    Ui::MainWindow *ui = nullptr;
    SpectrumModel *spectrummodel = nullptr;//能谱模型
    DataAnalysisWidget *dataAnalysisWidget = nullptr;//数据解析
    CommandHelper *commandhelper = nullptr;//网络指令
    qint32 currentDetectorIndex = 0;
};
#endif // MAINWINDOW_H
