#ifndef SYSUTILS_H
#define SYSUTILS_H

#include <vector>
#include <queue>
#include <deque>
#include <iostream>

using namespace std;

//fpga时间+能量
struct TimeEnergy{
    unsigned long long time; // 单位ns，八个字节int
    unsigned short energy; // 单位暂无,两个字节无符号数
    TimeEnergy(){
        this->time = 0;
        this->energy = 0;
    }
    TimeEnergy(unsigned long long time, unsigned short energy){
        this->time = time;
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
    std::deque<StepTimeEnergy> timeEnergy;
};

//typedef struct tagPariticalCountFrame{
//    unsigned long long channel;
//    unsigned long stepT;
//    unsigned long long dataT;
//    unsigned short dataE;
//}PariticalCountFrame;


//typedef struct tagPariticalSpectrumFrame{
//    unsigned long long channel;
//    unsigned long stepT;
//    std::vector<unsigned long long> dataT;
//    std::vector<unsigned short> dataE;
//}PariticalSpectrumFrame;

// 根据输入能量数据，绘制出能谱，
// data:能量点数组
// maxEnergy: 多道中最大道址对应的能量值。
// ch: 多道道数
// vector<unsigned short> GetSpectrum(const vector<unsigned short>& data, unsigned int maxEnergy=8192, int ch=4096);

// 计算data中数据在指定能量区间的数据个数
// data：输入的能量数据点
// leftE：左区间
// rightE：右区间
// stepT统计的时间步长, 单位s(底层固定=1，应用层自己根据时长刷新数据）
// return double：计数率,cps
double GetCount(const vector<unsigned short> &data, int leftE, int rightE, int stepT = 1);

// count  待拟合数据长度
// sx 待拟合数据x数组，
// sy 待拟合数据y数组
// 返回值result, result[0]为半高宽FWHM,result[1] 为峰位，result[2]为峰值。
// return, 拟合是否成功
// 待拟合函数：y(x) = a*exp[-4ln2(x-u)^2/FWHM^2]，a=result[2],u=result[1],FWHM=result[0].
// bool fit_GaussCurve(int count, std::vector<double> sx, std::vector<double> sy, double* result);

class SysUtils
{
public:
    SysUtils();
};

#endif // SYSUTILS_H
