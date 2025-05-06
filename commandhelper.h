/*
 * @Author: MrPan
 * @Date: 2025-04-06 20:15:30
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-05-06 17:42:36
 * @Description: 用来管理网口的数据发送与接受，管理网口数据的处理相关业务。
 */
#ifndef COMMANDHELPER_H
#define COMMANDHELPER_H

#include <QObject>
#include <QTcpSocket>
#include <QMutex>
#include <QFile>
#include <QElapsedTimer>

#include "qlitethread.h"
#include "sysutils.h"
#include "coincidenceanalyzer.h"

#define MAX_BYTEARRAY_SIZE (100 * 1024 * 1024) // 100 MB

class QLiteThread;
class CommandHelper : public QObject
{
    Q_OBJECT
public:
    explicit CommandHelper(QObject *parent = nullptr);
    ~CommandHelper();

    static CommandHelper *instance() {
        static CommandHelper commandHelper;
        return &commandHelper;
    }

    //实现容量检查与自动清空功能
    static bool checkAndClearQByteArray(QByteArray &data); 
    
    void initCommand();//初始化常用指令
    void startWork();
    void switchToCountMode(bool refModel);
    void updateStepTime(int stepT); //用于响应主界面点击刷新按钮
    /**
     * @description: 更新能窗和步长，适用于点击开始测量后，将参数传递给命令管理类
     * @param {int} stepT 时间步长，单位s
     * @param {unsigned short} EnWin 能窗。无单位
     * @param {bool} autoEnWindow 是否自动更新能窗，用于修正峰位漂移，比如常见的温漂。自动采用上一次的能窗进行高斯拟合，拟合后给出新的能创。
     * @return {*}
     */    
    void updateParamter(int stepT, unsigned short EnWin[4], bool _autoEnWindow);
    
    void exportFile(QString);
    void setDefaultCacheDir(QString dir);
    
    //设置测量发次号，用于文件夹分类
    inline void setShotNumber(QString _shotNum)
    {
        shotNum = _shotNum;
    }

    bool isConnected();//探测器是否连接
    unsigned int readTimeStep(){
        return this->stepT;
    }

protected:
    void handleManualMeasureNetData();//处理手动测量网络数据
    void handleAutoMeasureNetData();//处理自动测量网络数据

    enum WorkStatusFlag {
        NoWork = 0,     // 未开始
        Preparing = 1,  // 准备过程中...
        Measuring = 2,  // 测量中...
        Waiting = 3,  // 测量中...
        WorkEnd = 4    // 测量结束
    };

    enum MeasureModelFlag {
        None = 0,           // 无
        SpectrumModel = 1,  // 能谱
        PariticalModel = 2,  // 粒子
        WaverformModel = 3    // 波形
    };

    QMap<qint8, double> gainValue;

signals:
    // 继电器状态
    void sigRelayStatus(bool on);
    void sigRelayFault();//故障，一般指网络不通

    // 探测器状态
    void sigDetectorStatus(bool on);
    void sigDetectorFault();//故障，一般指网络不通

    void sigRefreshCountData(quint8, StepTimeCount);// 计数
    // void sigRefreshSpectrum(quint8, StepTimeEnergy);// 能谱
    // void sigRefreshConformData(StepTimeEnergy);// 能谱

    void sigCoincidenceResult(quint32, CoincidenceResult);
    void sigSingleSpectrum(SingleSpectrum);
    void sigCurrentPoint(quint32, CurrentPoint);
    void sigPlot(SingleSpectrum, vector<CoincidenceResult>);

    void sigDoTasks();
    void sigAnalyzeFrame();

    //测量正式开始/等待/停止
    void sigMeasureStart(qint8 mmode, qint8 tmode); //测量模式（手动、自动、标定），传输模式（能谱、波形、符合测量）
    void sigMeasureWait();
    void sigMeasureStop();

    //网络包数大小
    void sigRecvDataSize(qint32);
    void sigRecvPkgCount(qint32);

    //更新自动能宽
    void sigUpdateAutoEnWidth(std::vector<unsigned short>);

private:
    QByteArray frame;
    QByteArray cachePool;
    QByteArray handlerPool;
    QMutex mutexCache;//缓冲池交换网络数据所用 cachePool
    QMutex mutexPlot;//缓冲池交换帧数据所用 spectrumFrameCachePool
    QMutex mutexReset;//更新数据所用，一般用于开始测量，需要重置数据项
    quint32 SequenceNumber;// 帧序列号
    QLiteThread* analyzeNetDataThread;//处理网络数据线程，将数据进行解析成时间能量队
    QLiteThread* plotUpdateThread;//能谱信息处理线程
    quint32 currentFPGATime = 0;// FPGA当前时刻，单位：s
    bool autoChangeEnWindow = false; //是否自动适应能窗，用以修正温漂
    quint64 time_SetEnWindow = 0;// 记录下手动测量下，用户设置能窗的时间戳，单位：s

    CoincidenceAnalyzer* coincidenceAnalyzer;

    void saveParticleInfo(const vector<TimeEnergy>& data1_2, const vector<TimeEnergy>& data2_2);

    void doEnWindowData(SingleSpectrum r1, vector<CoincidenceResult> r3);
 
    static void analyzerRealCalback(SingleSpectrum r1, vector<CoincidenceResult> r3, void *callbackUser);
    // 暂时弃用该函数
    void analyzerCalback(SingleSpectrum r1, vector<CoincidenceResult> r3);       
public slots:
    void openRelay(bool first = false);
    void closeRelay();

    void openDetector();
    void closeDetector();

    //触发阈值1
    QByteArray getCmdTriggerThold1(quint16 ch1, quint16 ch2);

    //波形极性
    QByteArray getCmdWaveformPolarity(quint8 v = 0x00);

    //波形触发模式
    QByteArray getCmdWaveformTriggerModel(quint8 mode = 0x00);

    //波形长度
    QByteArray getCmdWaveformLength(quint8 v = 0x04);

    //能谱模式/粒子模式死时间
    QByteArray getCmdDeadTime(quint16 deadTime);

    //能谱刷新时间
    QByteArray getCmdSpectnumRefreshTimeLength(quint32 refreshTimelength);

    //探测器增益
    QByteArray getCmdDetectorGain(quint8 ch1, quint8 ch2, quint8 ch3, quint8 ch4);
    
    //是否梯形成形
    QByteArray getCmdDetectorTS_flag(bool flag);

    //梯形成形 上升沿、平顶、下降沿参数
    QByteArray getCmdDetectorTS_PointPara(quint8 rise, quint8 peak, quint8 fall);

    //梯形成形 时间常数，time1,time2
    QByteArray getCmdDetectorTS_TimePara(quint16 time1, quint16 time2);
    
    //梯形成形 基线的噪声下限
    QByteArray getCmdDetectorTS_BaseLine(quint16 baseLineLowLimit);

    //传输模式
    QByteArray getCmdTransferModel(quint8 mode);

    //开始手工测量
    void slotStartManualMeasure(DetectorParameter p);
    //停止手工测量
    void slotStopManualMeasure();

    //开始自动测量
    void slotStartAutoMeasure(DetectorParameter p);
    void slotStopAutoMeasure();

public:
    void netFrameWorkThead();
    void detTimeEnergyWorkThread();

private:
    QTcpSocket *socketRelay;    //继电器
    QTcpSocket *socketDetector;//探测器

    QByteArray cmdSoftTrigger;//软件触发，开始测量
    QByteArray cmdHardTrigger;//硬件触发，开始测量
    QByteArray cmdStopTrigger;//停止测量
    QByteArray cmdExternalTrigger;//外触发信号，硬件触发信号反馈
    QVector<QByteArray> cmdPool;

    WorkStatusFlag workStatus = NoWork;
    DetectorParameter detectorParameter;
    //QFile *pfSaveInvalid = nullptr;//保存非0有效数据文件
    QFile *pfSaveNet = nullptr;//保存开始测量之后的所有网口接收数据
    QFile *pfSaveVaildData = nullptr;//保存开始测量之后的所有有效数据[时间、死时间、能量] [6字节、2字节、2字节]

    qint8 prepareStep = 0;
    void initSocket(QTcpSocket** socket);
    bool taskFinished = false;
    bool sendStopCmd = false;
    bool detectorException = false;

private:
    QString defaultCacheDir;
    QString shotNum; //测量发次，用来作为文件前缀。
    QString netDataFileName; //存储网口接收全部原始数据的文件名
    QString validDataFileName; // 存储有效数据的文件名

    int stepT = 1; //界面图像刷新时间，单位s
    unsigned short EnWindow[4]; // 探测器1左能窗、右能窗；探测器2左能窗、右能窗
    std::vector<unsigned short> autoEnWindow; // 自动测量反馈给界面的值：探测器1左能窗、右能窗；探测器2左能窗、右能窗

    qint64 lastClockT = 0;
    bool refModel = false;
    unsigned int maxEnergy = 8192;

    std::vector<DetTimeEnergy> currentSpectrumFrames;//网络新接收的能谱数据（一般指未处理，未分步长的数据）
    std::vector<DetTimeEnergy> cacheSpectrumFrames;
};


#endif // COMMANDHELPER_H
