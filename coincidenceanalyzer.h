// 2025年3月20日16:02:38
// write by maoxiaoqing

#ifndef COINCIDENCEANALYZER_H
#define COINCIDENCEANALYZER_H
#include <vector>
#include <algorithm> // 包含 std::find_if
#include <functional>// std::function
#include <mutex>

// #define MULTI_CHANNEL 8192 //能谱多道的道数
constexpr unsigned short MULTI_CHANNEL = 8192;
#define MAX_ENERGY 8192 //最大能量
// long long nanoseconds = 1000000000LL; //1s = 1E9ns
#define NANOSECONDS 1e9
#define MAX_REMAIN_SPECTRUM 3600 //最大存留能谱数目

using namespace std;

//fpga时间+能量
struct TimeEnergy{
    unsigned long long time; // 单位ns，八个字节int
    unsigned short dietime;// 死时间
    unsigned short energy; // 单位暂无,两个字节无符号数
    TimeEnergy(){
        this->time = 0;
        this->dietime = 0;
        this->energy = 0;
    }
    TimeEnergy(unsigned long long time, unsigned short dietime, unsigned short energy){
        this->time = time;
        this->dietime = dietime;
        this->energy = energy;
    };
};

//通道+(fpga时间+能量...序列)
struct DetTimeEnergy{
    unsigned char channel;
    std::vector<TimeEnergy> timeEnergy;
};

//步长+计数
struct StepTimeCount{
    unsigned long long time; // 单位s
    unsigned short count; // 计数
    StepTimeCount(){}
    StepTimeCount(unsigned long long time, unsigned short count){
        this->time = time;
        this->count = count;
    }
};
//步长+能量
struct StepTimeEnergy{
    unsigned long long time; // 单位s
    vector<unsigned short> energy; // 单位暂无,两个字节无符号数
};

//通道+(步长+能量/计数...序列)
struct DetStepTimeEnergy{
    unsigned char channel;
    std::vector<StepTimeEnergy> timeEnergy;
};

// 符合计数的结果
struct CoincidenceResult{
    int ConCount_single = 0;//一秒内的单符合的个数
    int ConCount_multiple = 0;//一秒内的多符合的个数
    int CountRate1 = 0; //探测器1的计数率，是指在能窗内的计数率
    int CountRate2 = 0; //探测器2的计数率，是指在能窗内的计数率
};

struct CurrentPoint{
    int dataPoint1 = 0; //这次处理的探测器1数据点数
    int dataPoint2 = 0; //这次处理的探测器2数据点数
};

// 包大小65536 Byte
struct SingleSpectrum{ //存放每秒钟的能谱数据。1s产生一个能谱
    unsigned int time; //时刻(单位s)，以FPGA时钟进行计时，给出当前的能谱对应的时刻，第1个谱对应time=1。
    unsigned int spectrum[2][MULTI_CHANNEL] = {0, 0}; //探测器1能谱
};

struct AutoGaussFit{
    int time; //修改时间，单位s
    unsigned short EnLeft1;  // 探测器1符合能窗左边界
    unsigned short EnRight1; // 探测器1符合能窗右边界
    unsigned short EnLeft2;  // 探测器2符合能窗左边界
    unsigned short EnRight2; // 探测器2符合能窗右边界
};

class CoincidenceAnalyzer
{
public:
    CoincidenceAnalyzer();

    //重新初始化，每次从零时刻开始处理数据时，需要初始化
    inline void initialize(){
        countCoin = 0;
        // 清除向量中的所有元素并释放内存空间
        vector<CoincidenceResult>().swap(coinResult);

        vector<SingleSpectrum> emptySpec;
        swap(AllSpectrum, emptySpec);

        vector<CurrentPoint>().swap(AllPoint);

        vector<TimeEnergy>().swap(unusedData1);
        vector<TimeEnergy>().swap(unusedData2);
        vector<AutoGaussFit>().swap(GaussFitLog);

        //自动调整能窗相关参数重新初始化
        autoFirst = true;
        for(int i=0; i<MULTI_CHANNEL; i++)
        {
            GaussFitSpec.time = 0;
            GaussFitSpec.spectrum[0][i] = 0;
            GaussFitSpec.spectrum[1][i] = 0;
            AccumulateSpectrum.spectrum[0][i] = 0;
            AccumulateSpectrum.spectrum[1][i] = 0;
        }
    }

    //已经处理的计数率点数，也就是FPGA数据，已经处理了的时间长度，单位:秒
    inline int GetCountCoin(){return countCoin;}

    // 读取数据
    inline vector<CoincidenceResult> GetCoinResult(){
        return coinResult;
    }

    inline vector<SingleSpectrum> GetSpectrum(){
        return AllSpectrum;
    }

    //获取累计能谱
    inline SingleSpectrum GetAccumulateSpectrum()
    {
        return AccumulateSpectrum;
    }

    inline vector<CurrentPoint> GetPointPerSeconds(){
        return AllPoint;
    }

    inline vector<AutoGaussFit> GetGaussFitLog(){
        return GaussFitLog;
    }

    inline void GetEnWidth(std::vector<unsigned short>& EnWindow){
        EnWindow.push_back(EnergyWindow[0]);
        EnWindow.push_back(EnergyWindow[1]);
        EnWindow.push_back(EnergyWindow[2]);
        EnWindow.push_back(EnergyWindow[3]);
    }

    /// @brief 计算给出能谱曲线、计数曲线，计算的结果放在类成员中
    /// @param vector<TimeEnergy> data1 探测器1[时间(ns),能量]数据
    /// @param vector<TimeEnergy> data2 探测器2[时间(ns),能量]数据
    /// @param unsigned short E_win[4] 能窗，探测器1左、右能窗，探测器2左右能窗
    /// @param int windowWidthT 时间窗，单位ns
    /// @param int delayTime 符合延迟时间，单位ns。这里特征二通道相对于一通道的延迟。
    /// @param bool countFlag 是否计算计数曲线
    /// @param bool autoEnWidth 是否自动修正能窗
    void calculate(vector<TimeEnergy> data1, vector<TimeEnergy> data2,
            unsigned short E_win[4], int windowWidthT, int delayTime=0,
            bool countFlag=true, bool autoEnWidth = false);

    //这里加入回调函数，后期做成SDK会出现问题，SDK不存在回调，只存在返回值。
    void set_callback(std::function<void(SingleSpectrum, vector<CoincidenceResult>)> func);

    //C/C++标准回调导出
    typedef void (*pfRealCallbackFunction)(SingleSpectrum, vector<CoincidenceResult>, void *callbackUser);
    void set_callback(pfRealCallbackFunction func, void *callbackUser);
private:
    // 统计给出两个探测器各自当前一秒钟测量数据的能谱，当前一秒钟没有测量信号，则能谱全为零
    void calculateAllSpectrum(vector<TimeEnergy> data1, vector<TimeEnergy> data2);
    
    //自动能窗更新，根据初始的能窗，每到能谱数据累积满一定的数据量后， 进行高斯拟合，拟合得到的高斯半高宽作为新的能窗。
    void AutoEnergyWidth();

    //进行符合事件处理
    void Coincidence(vector<TimeEnergy> data1, vector<TimeEnergy> data2,
          unsigned short EnWindowWidth[4],
          int windowWidthT, int delayTime);

    // 根据输入能量数据，绘制出能谱，
    // data:能量点数组
    // maxEnergy: 多道中最大道址对应的能量值。
    // ch: 多道道数
    vector<int> GetSingleSpectrum(const vector<int>& data, int maxEnergy, unsigned short ch);
    
    //统计给出当前一秒内的两个探测器各自数据点的个数
    bool GetDataPoint(vector<TimeEnergy> data1, vector<TimeEnergy> data2);

    //统计给出直方图分布，类似matlab中的hist。
    vector<int> computeHistogram(const vector<int>& data, const vector<int>& binEdges);

    //找到不大于value的最后一个数的下标。返回是否查找到的标记
    bool find_index_above(vector<TimeEnergy> data, unsigned long long value, int& index);

    //找到第一个小于value的下标。返回是否查找到的标记
    bool find_index_below(vector<TimeEnergy> data, unsigned long long value, int& index);

private:
    int countCoin; //符合计数曲线的数据点个数,一个数据点对应1s.
    vector<CoincidenceResult> coinResult; //将所有处理的数据都存放在这个容器中，FPGA时钟每一秒钟产生一个点
    vector<SingleSpectrum> AllSpectrum;  //将处理的能谱数据都存放在这个容器中，FPGA时钟每一秒钟产生一个能谱，只存放最近一个小时的能谱。单端队列
    SingleSpectrum AccumulateSpectrum; //累积能谱,将每秒的能谱进行累积后的能谱
    vector<CurrentPoint> AllPoint; //每一秒内的各探测器的数据点个数
    vector<TimeEnergy> unusedData1, unusedData2;//用于存储未处理完的数据信息
    std::function<void(SingleSpectrum, vector<CoincidenceResult>)> m_pfunc = nullptr;
    pfRealCallbackFunction m_pfuncEx = nullptr;
    void* m_pfuncUser = nullptr;
    mutex mtx;
    
    // 用于高斯拟合，自动更新能窗
    bool autoFirst; //自动调节符合计算能窗宽度，用于解决峰飘问题，每测量一定数据点之后，自动拟合出高斯曲线，以新的半高宽来作为符合计算能窗。
    SingleSpectrum GaussFitSpec; // 用于高斯拟合的能谱，每秒钟汇总一次，并且计算能窗内计数点是否到达指定的点数。满足点数后便进行拟合，更新能谱
    int GaussCountMin; //自动高斯拟合的最小数据点数
    int GaussMinGapTime; //自动高斯拟合的能窗最小时间间隔,单位：s
    unsigned short EnergyWindow[4];//能窗边界，依次存放探测器1左边界、右边界，探测器2左边界、右边界。
    vector<AutoGaussFit> GaussFitLog; //记录修改能窗区间的左右宽度,
};
#endif // COINCIDENCEANALYZER_H
