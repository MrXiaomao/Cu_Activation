#include "sysutils.h"

SysUtils::SysUtils()
{

}

// 计算直方图
vector<int> computeHistogram(const vector<unsigned int>& data, const vector<int>& binEdges) {
    vector<int> histogram(binEdges.size() - 1, 0); // 初始化直方图，大小为 binEdges.size() - 1

    // 遍历数据，统计每个 bin 的频数
    for (int value : data) {
        // 找到 value 所属的 bin
//        auto it = std::lower_bound(binEdges.begin(), binEdges.end(), value);
//        int binIndex = it - binEdges.begin() - 1;
        int binIndex = value /(binEdges.at(1)-binEdges.at(0));
        // 确保 value 在 binEdges 的范围内
        if (binIndex >= 0 && binIndex < (int)histogram.size())
        {
            histogram[binIndex]++;
        }
    }

    return histogram;
}

// 根据输入能量数据，绘制出能谱，
// data:能量点数组
// maxEnergy: 多道中最大道址对应的能量值。
// ch: 多道道数
vector<int> GetSpectrum(const vector<unsigned int>& data, unsigned int maxEnergy, int ch)
{
    // 自动算出多道计数器的能量bin。
    vector<int> binEdges;
    int binWidth = maxEnergy/ch;
    for(int i=0; i< ch+1; i++){
        binEdges.push_back(binWidth*i);
    }
    // 计算能谱
    vector<int> spectrum = computeHistogram(data, binEdges);
    return spectrum;
}

// 计算data中数据在指定能量区间的数据个数
// data：输入的能量数据点
// leftE：左区间
// rightE：右区间
// return int：计数值
int GetCount(const vector<int> &data, int leftE, int rightE)
{
    int count = 0;
    for (int value : data)
    {
        if(value>leftE && value <=rightE) count++;
    }
    return count;
}

// 对输入的粒子[时间、能量]数据进行统计,给出计数率随时间变化的曲线，注意这里dataT与dataE一定是相同的长度
// dataT粒子的时间, 单位ns
// dataE粒子的能量
// stepT统计的时间步长, 单位s
// leftE：左区间
// rightE：右区间
// return：vector<TimeCountRate> 计数率数组
vector<TimeCountRate> GetCountRate(const vector<long long> &dataT, const vector<unsigned int> &dataE, int stepT, int leftE, int rightE)
{
    long long delaT = stepT;
    delaT *= 1E9;//单位转化 s转化ns

    // 初始化计数率数组
    long long TimeMax = dataT.back(); //最大时间
    int CountSize = TimeMax/delaT + 1; //计数率点的个数
    vector<TimeCountRate> countRate; //全部初始化为0
    countRate.resize(CountSize);

    // 初始化
    for (int i=0; i<CountSize; i++)
    {
        countRate.at(i).time = i*stepT;
        countRate.at(i).CountRates = 0.0;
    }

    //统计计数
    for (size_t i=0; i<dataT.size(); i++)
    {
        int indexT = static_cast<int>(dataT.at(i)/delaT); //相除取整
        if(dataE.at(i)>leftE && dataE.at(i)<=rightE)
        countRate.at(indexT).CountRates +=1.0;
    }

    //转化为计数率cps
    for (std::vector<TimeCountRate>::iterator it = countRate.begin(); it != countRate.end(); ++it)
    {
        (*it).CountRates /= stepT;
    }

    return countRate;
}

// 打印直方图
void printHistogram(const vector<int>& binEdges, const vector<int>& histogram) {
    for (size_t i = 0; i < histogram.size(); i++) {
        cout << "Bin "<<i<<"[" << binEdges[i] << ", " << binEdges[i + 1] << "): " << histogram[i] << std::endl;
    }
}
