#ifndef COINCIDENCEANALYZER_H
#define COINCIDENCEANALYZER_H
#include <vector>
#include <iostream>
#include <algorithm> // 包含 std::find_if

using namespace std;

struct TimeEnergy{
    long long time; // 单位ns，八个字节int
    unsigned short energy; // 单位暂无,两个字节无符号数
    TimeEnergy(long long time, unsigned short energy){
        this->time = time;
        this->energy = energy;
    }
};

struct CoinResult{
//    bool time_correct = false; //时间纠正，如果当前不足1s的数据，则不处理本次数据，time_correct为true。
    int dataPoint1 = 0; //这次处理的探测器1数据点数
    int dataPoint2 = 0; //这次处理的探测器2数据点数
    int ConCount_single = 0;//一秒内的单符合的个数
    int ConCount_multiple = 0;//一秒内的多符合的个数
};


class CoincidenceAnalyzer
{
public:
    CoincidenceAnalyzer();

    CoinResult Coincidence(vector<TimeEnergy> data1, vector<TimeEnergy> data2,
                          unsigned short E_left, unsigned short E_right, int windowWidthT);

    //已经处理的计数率点数，也就是FPGA数据，已经处理了的时间长度，单位:秒
    int GetcountCoin(){return countCoin;}

    //重新初始化，每次从零时刻开始处理数据时，需要初始化
    inline void initialize(){
        countCoin = 0;
        vector<CoinResult>().swap(result); // 清除向量中的所有元素并释放内存空间
    }

    //找到不大于value的最后一个数的下标。返回是否查找到的标记
    bool find_index_above(vector<TimeEnergy> data,long long value, int& index);

    //找到第一个小于value的下标。返回是否查找到的标记
    bool find_index_below(vector<TimeEnergy> data, long long value, int& index);

private:
    int countCoin; //符合计数曲线的数据点个数,一个数据点对应1s.
    vector<CoinResult> result; //将所有处理的数据都存放在这个容器中，FPGA时钟每一秒钟产生一个点
};

#endif // COINCIDENCEANALYZER_H
