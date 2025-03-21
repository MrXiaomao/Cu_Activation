// 2025年3月20日16:02:38
// write by maoxiaoqing

#ifndef COINCIDENCEANALYZER_H
#define COINCIDENCEANALYZER_H
#include <vector>
#include <algorithm> // 包含 std::find_if
#include <queue>
#include <deque>
#include <functional>
#include "sysutils.h"
#include <mutex>

#define MULTI_CHANNEL 4096 //能谱多道的道数
#define MAX_ENERGY 8192 //最大能量
// long long nanoseconds = 1000000000LL; //1s = 1E9ns
#define NANOSECONDS 1e9
#define MAX_REMAIN_SPECTRUM 3600 //最大存留能谱数目

using namespace std;

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

struct SingleSpectrum{ //存放每秒钟的能谱数据。1s产生一个能谱
    int time; //时刻(单位s)，以FPGA时钟进行计时，给出当前的能谱对应的时刻，第1个谱对应time=1。
    int spectrum[2][MULTI_CHANNEL] = {0, 0}; //探测器1能谱
    //int spectrum2[MULTI_CHANNEL] = {0}; //探测器2能谱
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

    inline vector<CurrentPoint> GetPointPerSeconds(){
        return AllPoint;
    }

    void calculate(vector<TimeEnergy> data1, vector<TimeEnergy> data2,
            unsigned short E_left1, unsigned short E_right1,
            unsigned short E_left2, unsigned short E_right2,
            int windowWidthT);

    void set_callback(std::function<void(vector<SingleSpectrum>, vector<CurrentPoint>, vector<CoincidenceResult>)> func);

private:
    // 统计给出两个探测器各自当前一秒钟测量数据的能谱，当前一秒钟没有测量信号，则能谱全为零
    void calculateAllSpectrum(vector<TimeEnergy> data1, vector<TimeEnergy> data2);

    //进行符合事件处理
    void Coincidence(vector<TimeEnergy> data1, vector<TimeEnergy> data2,
          unsigned short E_left1, unsigned short E_right1,
          unsigned short E_left2, unsigned short E_right2,
          int windowWidthT);

    // 根据输入能量数据，绘制出能谱，
    // data:能量点数组
    // maxEnergy: 多道中最大道址对应的能量值。
    // ch: 多道道数
    vector<int> GetSingleSpectrum(const vector<int>& data, int maxEnergy, int ch);

    //统计给出当前一秒内的两个探测器各自数据点的个数
    void GetDataPoint(vector<TimeEnergy> data1, vector<TimeEnergy> data2);

    //统计给出直方图分布，类似matlab中的hist。
    vector<int> computeHistogram(const vector<int>& data, const vector<int>& binEdges);

    //找到不大于value的最后一个数的下标。返回是否查找到的标记
    bool find_index_above(vector<TimeEnergy> data,long long value, int& index);

    //找到第一个小于value的下标。返回是否查找到的标记
    bool find_index_below(vector<TimeEnergy> data, long long value, int& index);

private:
    int countCoin; //符合计数曲线的数据点个数,一个数据点对应1s.
    vector<CoincidenceResult> coinResult; //将所有处理的数据都存放在这个容器中，FPGA时钟每一秒钟产生一个点
    // vector<SingleSpectrum> AllSpectrum;
    vector<SingleSpectrum> AllSpectrum;  //将处理的能谱数据都存放在这个容器中，FPGA时钟每一秒钟产生一个能谱，只存放最近一个小时的能谱。单端队列
    vector<CurrentPoint> AllPoint; //每一秒内的各探测器的数据点个数
    vector<TimeEnergy> unusedData1, unusedData2;//用于存储未处理完的数据信息
    std::function<void(vector<SingleSpectrum>, vector<CurrentPoint>, vector<CoincidenceResult>)> m_pfunc;
    mutex mtx;
};
#endif // COINCIDENCEANALYZER_H
