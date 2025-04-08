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
typedef struct tagDetectorParameter{
    // 触发阈值
    qint16 triggerThold1, triggerThold2;

    // 波形极性
    /*
        00:正极性
        01:负极性
        默认正极性
    */
    qint8 waveformPolarity;


    // 能谱模式/粒子模式死时间 单位ns
    qint16 deadTime;
    // 能谱刷新时间
    quint32 refreshTimeLength;
    //波形长度
    quint32 waveLength;
    //波形触发模式
    qint8 triggerModel;

    // 探测器增益
    qint8 gain;

    // 传输模式
    /*
    00:能谱
    03:波形
    05:粒子模式
    */
    qint8 transferModel;

    // 测量模式
    qint8 measureModel;//00-手动测量 01-自动测量 02-标定测量

    qint32 cool_timelength;//冷却时长
} DetectorParameter;

class NetWorker : public QObject
{
    Q_OBJECT
public:
    void push_back(QByteArray binaryData){
        QMutexLocker locker(&mutexCache);
        cachePool.push_back(binaryData);
    };
public slots:
    void doWork();

signals:
    void resultReady(const QString &result);
    void sigMeasureStop();
    //网络包数大小
    void sigRecvDataSize(qint32);
    void sigRecvPkgCount(qint32);

    //分拣完毕
    void sigDispatchEnergyPkg(DetTimeEnergy);

private:
    QMutex mutexCache;
    QByteArray cachePool;
    QByteArray handlerPool;
    DetectorParameter detectorParameter;
    QFile *pfSave = nullptr;
    bool taskFinished = false;
};

class EnergyWorker : public QObject
{
    Q_OBJECT
public slots:
    void doWork();

    void handleDispatchEnergyPkg(DetTimeEnergy detTimeEnergy){
        QMutexLocker locker(&mutexPlot);
        currentSpectrumFrames.push_back(detTimeEnergy);
    }
signals:
    void resultReady(const QString &result);

private:
    QMutex mutexPlot;
    std::vector<DetTimeEnergy> currentSpectrumFrames;//网络新接收的能谱数据（一般指未处理，未分步长的数据;
};


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

    void startWork();
    void switchShowModel(bool refModel);
    void updateParamter(int stepT, int EnWin[4], int timewidth = 50, bool reset = false);
    void saveFileName(QString);
    void setDefaultCacheDir(QString dir);
    bool isConnected();//探测器是否连接

    enum Detector{
        Detector_1 = 0,
        Detector_2 = 1,
        Detector_Count = 2
    };

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

private:
    QByteArray frame;
    QByteArray command;
    QByteArray cachePool;
    QByteArray handlerPool;
    QMutex mutexCache;//缓冲池交换网络数据所用 cachePool
    QMutex mutexPlot;//缓冲池交换帧数据所用 spectrumFrameCachePool
    QMutex mutexReset;//更新数据所用，一般用于开始测量，需要重置数据项
    quint32 SequenceNumber;// 帧序列号
    QLiteThread* analyzeNetDataThread;
    QLiteThread* plotUpdateThread;

    QThread netWorkerThread;
    QThread energyWorkerThread;
    NetWorker *netWorker;
    EnergyWorker *energyWorker;

    CoincidenceAnalyzer* coincidenceAnalyzer;
    //void analyzerCalback(deque<SingleSpectrum>, vector<CurrentPoint>, vector<CoincidenceResult>);
signals:
    void operate();

protected:
    void makeFrame();
    quint8 crc();

public slots:
    void openRelay();
    void closeRelay();

    void openDetector();
    void closeDetector();

    //触发阈值1
    void slotTriggerThold1(quint16 ch1, quint16 ch2);

    //触发阈值2
    void slotTriggerThold2(quint16 ch3, quint16 ch4);

    //波形极性
    void slotWaveformPolarity(quint8 v = 0x00);

    //波形触发模式
    void slotWaveformTriggerModel(quint8 mode = 0x00);

    //波形长度
    void slotWaveformLength(quint8 v = 0x04);

    //能谱模式/粒子模式死时间
    void slotDeadTime(quint16 deadTime);

    //能谱刷新时间
    void slotSpectnumRefreshTimeLength(quint32 refreshTimelength);

    //探测器增益
    void slotDetectorGain(quint8 ch1, quint8 ch2, quint8 ch3, quint8 ch4);

    //传输模式
    void slotTransferModel(quint8 mode);

    //开始测量
    void slotStart(qint8 mode);

    //开始手工测量
    void slotStartManualMeasure(DetectorParameter p);
    //停止手工测量
    void slotStopManualMeasure();

    //开始自动测量
    void slotStartAutoMeasure(DetectorParameter p);
    void slotStopAutoMeasure();

public slots:
    void slotDoTasks();
    void slotAnalyzeNetFrame();
    void slotPlotUpdateFrame();

private:
    QTcpSocket *socketRelay;    //继电器
    QTcpSocket *socketDetector;//探测器

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
    int EnWindow[4]; // 探测器1左能窗、右能窗；探测器2左能窗、右能窗
    int timeWidth = 50;//时间窗宽度，单位ns(符合分辨时间)
    qint64 lastClockT = 0;
    bool refModel = false;
    unsigned int maxEnergy = 8192;

    std::vector<DetTimeEnergy> currentSpectrumFrames;//网络新接收的能谱数据（一般指未处理，未分步长的数据）
};


#endif // COMMANDHELPER_H
