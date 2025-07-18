/*
 * @Author: MrPan
 * @Date: 2025-04-20 09:21:32
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-07-18 21:22:18
 * @Description: 请填写简介
 */
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
    bool LoadMeasureParameter(QString filePath);
    void analyse(DetectorParameter detPara, unsigned int start_time, unsigned int time_end);
    struct yieldResult
    {
        int time; //时间s
        double ActiveOmiga; //初始相对活度
        double Yield; //中子产额
        /* data */
    };
    
    /**
     * @description: 检测文件路径是否有效
     * @param {QString} &path
     * @param {bool} checkExists
     * @param {bool} checkWritable
     * @return {*}true:路径有效，ffalse:路径无效
     */
    static bool isValidFilePath(const QString &path, bool checkExists = true, bool checkWritable = false);

    /**
     * @description: 检测文件是否已经存在
     * @param {QString} &filePath
     * @return {*}true:文件已经存在
     */
    static bool fileExists(const QString &filePath);

    /**
     * @description: 实现智能追加文件后缀的功能：如果文件名没有.txt后缀，则自动追加；如果已有则不追加。
     * @param {QString} &fileName
     * @return {*}
     */
    static QString smartAddTxtExtension(const QString &fileName);
    //读取配置文件
    void loadConfig();

signals:
    void sigNewPlot(SingleSpectrum, vector<CoincidenceResult>);
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

    /**
     * @description: 保存解析数据
     * @return {*}
     */
    void on_pushbutton_save_clicked();

    void on_checkBox_gauss_stateChanged(int arg1);

private:
    Ui::OfflineDataAnalysisWidget *ui;
    QString validDataFileName;
    CoincidenceAnalyzer* coincidenceAnalyzer = nullptr;
    DetectorParameter detParameter;

    quint32 stepT = 1;
    unsigned int startTime_absolute; //记录保存数据的起始时刻(冷却时间+FPGA时钟)。单位s，FPGA内部时钟
    unsigned int startFPGA_time; //开始保存FPGA有效数据的时刻(FPGA时钟),在这之前计数值都为零
    bool reAnalyzer = false; // 是否重新开始解析
    bool analyzerFinished = true;// 解析是否完成
    quint64 detectNum = 0; //探测到的粒子数
    
    std::map<unsigned int, unsigned long long> lossData; //记录丢包的时刻（FPGA时钟，单位:s）以及丢失的时间长度(单位：ns)。

    bool firstPopup = true;//控制弹窗是否首次出现
    SingleSpectrum totalSingleSpectrum; //总能谱
    vector<CoincidenceResult> coinResult; //符合计算结果

    int maxTime_updateYield = 3600; //自动更新中子产额的最大时刻，单位s
    int deltaTime_updateYield = 60; //自动更新中子产额的时间间隔，单位s
    bool isUpdateYield = false; //是否定时更新中子产额
    std::vector<yieldResult> activationResult; //计算结果，当设置为自动更新中子产额时，则会产生多个中子产额。

    unsigned int startTimeUI = 0;
    unsigned int endTimeUI = 0;
};

#endif // OFFLINEDATAANALYSISWIDGET_H
