/*
 * @Author: MrPan
 * @Date: 2025-04-06 20:15:30
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-08-22 17:26:03
 * @Description: 用来管理网口的数据发送与接受，管理网口数据的处理相关业务。
 */
#ifndef COMMANDHELPER_H
#define COMMANDHELPER_H

#include <QObject>
#include <QTcpSocket>
#include <QMutex>
#include <QFile>
#include <QElapsedTimer>
#include <QWaitCondition>
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
    void loadConfig(); //加载配置文件user.json
    
    // 获取有效数据的文件名
    inline const QString getFilenamePrefix(){
        QString prefix_name = ShotDir + "/" + str_start_time;
        return prefix_name;
    }

    //获取测量参数
    inline const DetectorParameter getDetParameter(){
        return detectorParameter;
    }

    //获取增益表
    inline const QMap<qint8, double> getGainValue()
    {
        return gainValue;
    }

    //获取设置能窗时的起始时刻
    inline const quint32 getsetEnTime()
    {
        return time_SetEnWindow;
    }

    // 距离打靶时刻的时长，单位：s
    inline const int getcurrentTimeToShot()
    {
        return currentTimeToShot;
    }
    /**
     * @description: 更新能窗和步长，适用于点击开始测量后，将参数传递给命令管理类
     * @param {int} stepT 时间步长，单位s
     * @param {unsigned short} EnWin 能窗。无单位
     * @param {bool} autoEnWindow 是否自动更新能窗，用于修正峰位漂移，比如常见的温漂。自动采用上一次的能窗进行高斯拟合，拟合后给出新的能创。
     * @return {*}
     */
    void updateParamter(int stepT, double EnWin[4], bool _autoEnWindow);
    
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
        // WorkSetting = 5 //配置参数中...
    };

    enum MeasureModelFlag {
        None = 0,           // 无
        SpectrumModel = 1,  // 能谱
        PariticalModel = 2,  // 粒子
        WaverformModel = 3    // 波形
    };

    QMap<qint8, double> gainValue;

signals:
    //更新界面日志
    void sigAppendMsg2(const QString &msg, QtMsgType msgType);
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
    
    //对于计数曲线，每次只添加一个点，减少大容器的数据拷贝耗时
    void sigPlot(SingleSpectrum, CoincidenceResult);
    //刷新整个曲线
    void sigNewPlot(SingleSpectrum, vector<CoincidenceResult>);

    void sigDoTasks();
    void sigAnalyzeFrame();

    //测量正式开始/等待/停止
    void sigMeasureStart(qint8 mmode, qint8 tmode); //测量模式（手动、自动、标定），传输模式（能谱、波形、符合测量）
    void sigMeasureWait();
    void sigMeasureStop();
    void sigAbonormalStop();
    void sigMeasureStopWave();
    void sigMeasureStopSpectrum();
    void sigUpdate_Active_yield(double, double); //刷新相对活度、中子产额

    //网络包数大小
    void sigRecvDataSize(qint32);
    void sigRecvPkgCount(qint32);

    /**
     * @description: 更新自动能宽
     * @param {unsigned} short 能窗
     * @param {qint8} mmodel 测量模式，自动、手动
     * @return {*}
     */
    void sigUpdateAutoEnWidth(std::vector<double>, qint8 mmodel);

    //测量计时开始
    void sigMeasureTimerStart(qint8 mmode, qint8 tmode, QDateTime);

public:
    //手动测量中，确认能窗后，开始存储有效数据文件。
    bool startSaveValidData = false;
    
private:
    bool ready = false;
    QWaitCondition condition;
    QByteArray frame;
    QByteArray dataBuffer; //网口接收数据
    QByteArray cachePool;
    QByteArray handlerPool;
    QMutex mutexCache;//缓冲池交换网络数据所用 cachePool
    QMutex mutexPlot;//缓冲池交换帧数据所用 spectrumFrameCachePool
    QMutex mutexReset;//更新数据所用，一般用于开始测量，需要重置数据项
    QMutex mutexFile;//写文件锁    
    
    quint32 SequenceNumber;// 数据帧序列号
    quint64 lastTime; //用来修正FPGA掉包问题,记录正常包的末尾时间,单位ns 
    quint64 firstTime; //用来修正FPGA掉包问题，记录丢包之后的第一个有效时间，单位ns
    std::map<unsigned int, unsigned long long> lossData; //记录丢包的时刻（FPGA时钟，单位:s）以及丢失的时间长度(单位：ns)。

    QLiteThread* analyzeNetDataThread;//处理网络数据线程，将数据进行解析成时间能量队
    QLiteThread* plotUpdateThread;//能谱信息处理线程
    quint32 currentFPGATime = 0;// FPGA当前时刻，单位：s
    quint32 currentTimeToShot = 0;// 距离打靶时刻的时长，单位：s
    bool autoChangeEnWindow = false; //是否自动适应能窗，用以修正温漂
    quint32 time_SetEnWindow = 0;// 记录下手动测量下，用户设置能窗的时间戳，以活化后开始计时(冷却时间+FPGA时钟)，单位：s

    CoincidenceAnalyzer* coincidenceAnalyzer;

    void saveParticleInfo(const vector<TimeEnergy>& data1_2, const vector<TimeEnergy>& data2_2);

    void updateCoincidenceData(const SingleSpectrum &r1, const vector<CoincidenceResult>& r3);
    
    // 将相对活度转化为中子产额
    void activeOmigaToYield(double active);
 
    static void analyzerRealCalback(const SingleSpectrum &r1, const vector<CoincidenceResult> &r3, void *callbackUser);
    
    void saveConfigParamter();

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
    void slotStartAutoMeasure();
    void slotStopAutoMeasure();

public:
    void netFrameWorkThead();
    void detTimeEnergyWorkThread();
    void setAutoMeasureParameter(DetectorParameter p); // 设置自动测量参数

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

    qint8 prepareStep = 0;
    void initSocket(QTcpSocket** socket);
    bool taskFinished = false;
    bool sendStopCmd = false;
    bool detectorException = false;

private:
    QString ShotDir;
    QString defaultCacheDir;
    QString shotNum; //测量发次，用来作为文件前缀。
    QString str_start_time; //测量开始时间，作为文件名前缀
    QString netDataFileName; //存储网口接收全部原始数据的文件名
    QString validDataFileName; // 存储有效数据的文件名
    
    QFile *pfSaveNet = nullptr;//保存开始测量之后的所有网口接收数据
    QFile *pfSaveVaildData = nullptr;//保存开始测量之后的所有有效数据[时间、死时间、能量] [6字节、2字节、2字节]

    QByteArray m_netBuffer; //缓存网口数据，仅仅用于写文件
    QElapsedTimer NetDataTimer; //定时器，用于定时缓存网口数据，这意味着意外崩溃会丢失最后那一段数据。
    
    int stepT = 1; //界面图像刷新时间，单位s
    double EnWindow[4]; // 探测器1左能窗、右能窗；探测器2左能窗、右能窗
    std::vector<unsigned short> autoEnWindow; // 符合测量自动更新能窗反馈给界面的值：探测器1左能窗、右能窗；探测器2左能窗、右能窗

    int deltaTime_updateYield = 60; //中子产额的刷新时间间隔，单位s
    int maxTime_updateYield = 3600; //中子产额的最后一次刷新对应时间，单位s。超出该时间之后便不再刷新中子产额
    qint64 lastClockT = 0;
    bool refModel = false;
    unsigned int maxEnergy = 8192;
    int commandDelay = 15;  // 两条指令之间的间隔,ms，默认15ms，但可配置

    std::vector<DetTimeEnergy> currentSpectrumFrames;//网络新接收的能谱数据（一般指未处理，未分步长的数据）
    std::vector<DetTimeEnergy> cacheSpectrumFrames;
};

extern double enCalibration[2]; //能量刻度参数
extern unsigned int multiChannel; //多道道数
#endif // COMMANDHELPER_H
