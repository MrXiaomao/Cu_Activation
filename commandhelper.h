#ifndef COMMANDHELPER_H
#define COMMANDHELPER_H

#include <QObject>
#include <QTcpSocket>
#include <QMutex>
#include <QFile>

#include "sysutils.h"
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


    // 能谱模式/粒子模式死时间
    qint16 dieTimeLength;
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

    // 保存路径
    QString path;

    // 保存文件名
    QString filename;
} DetectorParameter;

class QUiThread;
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

    //void setDetectorParamter();
    void updateShowModel(bool refModel);
    void updateParamter(int stepT, int leftE[2], int rightE[2]);
    void saveFileName(QString);
    void setDefaultCacheDir(QString dir);

    enum WorkStatusFlag {
        NoWork = 0,     // 未开始
        Preparing = 1,  // 准备过程中...
        Measuring = 2,  // 测量中...
        WorkEnd = 3    // 测量结束
    };

    enum MeasureModelFlag {
        None = 0,           // 无
        SpectrumModel = 1,  // 能谱
        PariticalModel = 2,  // 粒子
        WaverformModel = 3    // 波形
    };

signals:
    // 位移平台状态
    void sigDisplacementStatus(bool on, qint32 index);
    void sigDisplacementFault(qint32 index);//故障，一般指网络不通

    // 继电器状态
    void sigRelayStatus(bool on);
    void sigRelayFault();//故障，一般指网络不通

    // 探测器状态
    void sigDetectorStatus(bool on);
    void sigDetectorFault();//故障，一般指网络不通

    void sigRefreshCountData(PariticalCountFrame);// 计数
    void sigRefreshSpectrum(PariticalSpectrumFrame);// 能谱

    void sigDoTasks();
    void sigAnalyzeFrame();

private:
    QByteArray frame;
    QByteArray command;
    QByteArray cachePool;
    QMutex mutex;
    QMutex mutexPlot;
    quint32 SequenceNumbre;// 帧序列号    
    QUiThread* analyzeNetDataThread;
    QUiThread* plotUpdateThread;

protected:
    void makeFrame();
    quint8 crc();

public slots:
//    void openPower(QString ip, qint32 port);
//    void closePower(QString ip, qint32 port);

    void openRelay(QString ip, qint32 port);
    void closeRelay();

    void openDisplacement(QString ip, qint32 port, qint32 index);
    void closeDisplacement(qint32 index);

    void openDetector(QString ip, qint32 port);
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
    void slotDieTimeLength(quint16 dieTimelength);

    //能谱刷新时间
    void slotSpectnumRefreshTimeLength(quint32 refreshTimelength);

    //探测器增益
    void slotDetectorGain(quint8 ch1, quint8 ch2, quint8 ch3, quint8 ch4);

    //传输模式
    void slotTransferModel(quint8 mode);

    //开始测量
    void slotStart();

    //开始手工测量
    void slotStartManualMeasure(DetectorParameter p);

    //开始能谱测量
    void slotStartSpectrumMeasure(DetectorParameter p);

    //停止手工测量
    void slotStopManualMeasure();

public slots:
    void slotDoTasks();
    void slotAnalyzeNetFrame();
    void slotPlotUpdateFrame();

private:
    QTcpSocket *socketDisplacement1;//2个位移平台、1个继电器、1个探测器
    QTcpSocket *socketDisplacement2;
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
    int stepT = 1, leftE[2], rightE[2];
    bool refModel = false;
    unsigned int maxEnergy = 8192;
    int maxCh = 4096;
    bool firstHandle = true;//是否第一次处理计数
    std::vector<PariticalSpectrumFrame> spectrumFrameCachePool;
    std::vector<PariticalCountFrame> countFrameCachePool;
};

#include <QThread>
typedef std::function<void()> LPThreadRunProc;
class QGuiThread :public QThread {
    Q_OBJECT
private:
    LPThreadRunProc m_func = 0;//记录线程执行的函数

public:
    //构造函数，func参数为线程执行的函数
    explicit QGuiThread(LPThreadRunProc func = NULL, QObject* parent = Q_NULLPTR) :QThread(parent),
        m_func(func)
    {
        connect(this, &QGuiThread::invokeSignal, this, &QGuiThread::invokeSlot);
    }

    ~QGuiThread()
    {
        disconnect(this, &QGuiThread::invokeSignal, this, &QGuiThread::invokeSlot);
    }

    void setThreadWorkProc(LPThreadRunProc func) {
        this->m_func = func;
    }

    void invoke(LPThreadRunProc func) {
        emit invokeSignal(func);
    }

protected:
    void run() override {
        m_func();
    }

signals:
    void invokeSignal(LPThreadRunProc fun);

public slots:
    void invokeSlot(LPThreadRunProc fun) {
        fun();
    }
};

typedef std::function<void()> LPThreadWorkProc;
class QUiThread :public QThread {
    Q_OBJECT

private:
    LPThreadWorkProc m_pfThreadWorkProc = 0;

public:
    explicit QUiThread(QObject* parent = Q_NULLPTR, LPThreadWorkProc pfThreadWorkProc = Q_NULLPTR)
        : QThread(parent)
        , m_pfThreadWorkProc(pfThreadWorkProc)
    {
        qDebug() << "thread create: " << this->objectName();
        connect(this, &QThread::finished, this, &QThread::deleteLater);
    }

    //析构函数
    ~QUiThread()
    {
        qDebug() << "thread exit: " << this->objectName();
    }

    void setWorkThreadProc(LPThreadRunProc pfThreadRun) {
        this->m_pfThreadWorkProc = pfThreadRun;
    }

protected:
    void run() override {
        m_pfThreadWorkProc();
    }

signals:
    //这里信号函数的参数个数可以根据自己需要随意增加
    void invokeSignal();
    void invokeSignal(QVariant);

public slots:

};

#endif // COMMANDHELPER_H
