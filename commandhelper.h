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

class QLiteThread;
class CommandHelper : public QObject
{
    Q_OBJECT
public:
    enum Measure_model{
        mmNone = 0x00,
        mmManual= 0x01, // 手动
        mmAuto = 0x02, // 自动
        mmDefine = 0x03, // 标定
    };

    explicit CommandHelper(QObject *parent = nullptr);
    ~CommandHelper();

    static CommandHelper *instance() {
        static CommandHelper commandHelper;
        return &commandHelper;
    }

    void initCommand();//初始化常用指令
    void startWork();
    void switchToCountMode(bool refModel);
    void updateStepTime(int stepT, int timewidth = 50);
    void updateParamter(int stepT, unsigned short EnWin[4], int timewidth = 50, bool reset = false);
    void saveFileName(QString);
    void setDefaultCacheDir(QString dir);
    bool isConnected();//探测器是否连接

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
    void sigRefreshSpectrum(quint8, StepTimeEnergy);// 能谱
    void sigRefreshConformData(StepTimeEnergy);// 能谱

    void sigCoincidenceResult(quint32, CoincidenceResult);
    void sigSingleSpectrum(SingleSpectrum);
    void sigCurrentPoint(quint32, CurrentPoint);
    void sigPlot(SingleSpectrum, vector<CoincidenceResult>, int refreshT);

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
    //QByteArray command;
    QByteArray cachePool;
    QByteArray handlerPool;
    QMutex mutexCache;//缓冲池交换网络数据所用 cachePool
    QMutex mutexPlot;//缓冲池交换帧数据所用 spectrumFrameCachePool
    QMutex mutexReset;//更新数据所用，一般用于开始测量，需要重置数据项
    quint32 SequenceNumber;// 帧序列号
    QLiteThread* analyzeNetDataThread;//处理网络数据线程，将数据进行解析成时间能量队
    QLiteThread* plotUpdateThread;//能谱信息处理线程
    quint32 currentEnergyTime = 0;// 能谱时间
    quint64 tmCalculate = 0;// 记录下重新运算的时间戳，单位：毫秒

    CoincidenceAnalyzer* coincidenceAnalyzer;
    void analyzerCalback(SingleSpectrum r1, vector<CoincidenceResult> r3);

public slots:
    void openRelay();
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
    QFile *pfSave = nullptr;
    qint8 prepareStep = 0;
    void initSocket(QTcpSocket** socket);
    bool taskFinished = false;

private:
    QString defaultCacheDir;
    QString currentFilename;
    int stepT = 1; //界面图像刷新时间
    unsigned short EnWindow[4]; // 探测器1左能窗、右能窗；探测器2左能窗、右能窗
    std::vector<unsigned short> autoEnWindow; // 探测器1左能窗、右能窗；探测器2左能窗、右能窗
    int timeWidth = 50;//时间窗宽度，单位ns(符合分辨时间)
    qint64 lastClockT = 0;
    bool refModel = false;
    unsigned int maxEnergy = 8192;

    std::vector<DetTimeEnergy> currentSpectrumFrames;//网络新接收的能谱数据（一般指未处理，未分步长的数据）
    std::vector<DetTimeEnergy> cacheSpectrumFrames;
};


#endif // COMMANDHELPER_H
