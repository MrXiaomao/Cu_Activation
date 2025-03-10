#ifndef COMMANDHELPER_H
#define COMMANDHELPER_H

#include <QObject>
#include <QTcpSocket>
#include <QMutex>

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

    // 探测器增益
    qint8 gain;

    // 传输模式
    /*
    00:能谱
    03:波形
    05:粒子模式
    */
    qint8 transferModel;
} DetectorParameter;

typedef struct tagPariticalData{
    quint64 time;
    quint16 data;
}PariticalData;
typedef struct tagPariticalFrame{
    quint64 channel;
    QVector<PariticalData> data;
}PariticalFrame;

class CommandHelper : public QObject
{
    Q_OBJECT
public:
    explicit CommandHelper(QObject *parent = nullptr);

    void setDetectorParamter();

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
    // 电源状态
    void sigPowerStatus(bool on);

    // 继电器状态
    void sigRelayStatus(bool on1, bool on2);

    // 探测器状态
    void sigDetectorStatus(bool on);

    // 位移平台状态
    //void sigDisplacementStatus(bool on1, bool on2);

    void sigRefreshData(PariticalFrame);

    void sigDoTasks();
    void sigAnalyzeFrame();

private:
    QByteArray frame;
    QByteArray command;
    QByteArray cachePool;
    QMutex mutex;
    quint32 SequenceNumbre;// 帧序列号
    PariticalFrame vctFrames;

protected:
    void makeFrame();
    quint8 crc();

public slots:
    void openPower(QString ip, qint32 port);
    void closePower(QString ip, qint32 port);

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
    void slotStartManualMeasure(QString ip, qint32 port, DetectorParameter p);

    //停止手工测量
    void slotStopManualMeasure();

public slots:
    void slotDoTasks();
    void slotAnalyzeFrame();

private:
    QTcpSocket *socketDisplacement1;//2个位移平台、1个继电器、1个探测器
    QTcpSocket *socketDisplacement2;
    QTcpSocket *socketRelay;
    QTcpSocket *socketDetector;
    WorkStatusFlag workStatus = NoWork;
    DetectorParameter detectorParameter;

    qint8 prepareStep = 0;
    void initSocket(QTcpSocket** socket);

    bool taskFinished = false;
private:

};

class Worker:public QObject                    //work定义了线程要执行的工作
{
    Q_OBJECT
public:
    Worker(QObject* parent = nullptr){}

public slots:
//    void slotDoTasks();
//    void slotAnalyzeFrame();

signals:
    void resultReady(const int result);               //线程完成工作时发送的信号
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
    explicit QUiThread(LPThreadWorkProc pfThreadWorkProc = Q_NULLPTR, QObject* parent = Q_NULLPTR)
        : QThread(parent)
        , m_pfThreadWorkProc(pfThreadWorkProc)
    {
        connect(this, &QThread::finished, this, &QThread::deleteLater);
    }

    //析构函数
    ~QUiThread()
    {

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
