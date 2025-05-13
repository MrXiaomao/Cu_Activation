#ifndef OFFLINEDATAANALYSISWIDGET_H
#define OFFLINEDATAANALYSISWIDGET_H

#include <QWidget>
#include "sysutils.h"
#include "coincidenceanalyzer.h"

namespace Ui {
class OfflineDataAnalysisWidget;
}

class OfflineDataAnalysisWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OfflineDataAnalysisWidget(QWidget *parent = nullptr);
    ~OfflineDataAnalysisWidget();

    void initCustomPlot();
    void doEnWindowData(SingleSpectrum r1, vector<CoincidenceResult> r3);
    void openEnergyFile(QString filePath);
    void analyse(DetectorParameter detPara, unsigned int start_time, unsigned int time_end);

signals:
    void sigPlot(SingleSpectrum, vector<CoincidenceResult>);
    void sigStart();
    void sigPausePlot(bool); //是否暂停图像刷新
    void sigEnd(bool);
    void sigActiveOmiga(double active); //计算出相对活度

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void slotStart();
    void slotEnd(bool);

private slots:
    void on_pushButton_start_clicked();
    void slotUpdateEnWindow();

private:
    Ui::OfflineDataAnalysisWidget *ui;
    QString validDataFileName;
    CoincidenceAnalyzer* coincidenceAnalyzer = nullptr;
    DetectorParameter detParameter;

    quint32 stepT = 1;
    unsigned int startTime_FPGA; //记录保存数据的FPGA起始时刻。单位s，FPGA内部时钟
    bool reAnalyzer = false; // 是否重新开始解析
    bool analyzerFinished = true;// 解析是否完成
    quint64 detectNum = 0; //探测到的粒子数

    std::map<unsigned int, unsigned long long> lossData; //记录丢包的时刻（FPGA时钟，单位:s）以及丢失的时间长度(单位：ns)。

    bool firstPopup = true;//控制弹窗是否首次出现
    SingleSpectrum totalSingleSpectrum;
};

#endif // OFFLINEDATAANALYSISWIDGET_H
