#ifndef SYSUTILS_H
#define SYSUTILS_H

#include <vector>
#include <iostream>

using namespace std;
struct TimeEnergy{
    long long time; // 单位ns
    int Energy; // 单位暂无
};

struct TimeCountRate{
    int time; //单位s
    double CountRates; //单位cps
};

typedef struct tagPariticalFrame{
    long long channel;
    std::vector<long long> dataT;
    std::vector<unsigned int> dataE;
    std::vector<TimeCountRate> timeCountRate;
}PariticalCountFrame;

typedef struct tagPariticalSpectrumFrame{
    long long channel;
    std::vector<long long> dataT;
    std::vector<unsigned int> dataE;
    std::vector<int> data;
}PariticalSpectrumFrame;

// 根据输入能量数据，绘制出能谱，
// data:能量点数组
// maxEnergy: 多道中最大道址对应的能量值。
// ch: 多道道数
vector<int> GetSpectrum(const vector<unsigned int>& data, unsigned int maxEnergy=8192, int ch=4096);

// 对输入的粒子[时间、能量]数据进行统计,给出计数率随时间变化的曲线，注意这里dataT与dataE一定是相同的长度
// dataT粒子的时间, 单位ns
// dataE粒子的能量
// stepT统计的时间步长, 单位s
// leftE：左区间
// rightE：右区间
// return：vector<TimeCountRate> 计数率数组
vector<TimeCountRate> GetCountRate(const vector<long long> &dataT, const vector<unsigned int> &dataE, int stepT, int leftE, int rightE);

// 打印直方图
void printHistogram(const vector<int>& binEdges, const vector<int>& histogram);

class SysUtils
{
public:
    SysUtils();
};

#endif // SYSUTILS_H
