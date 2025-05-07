/*
 * @Author: MaoXiaoqing
 * @Date: 2025-04-06 20:15:30
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-05-06 20:02:46
 * @Description: 请填写简介
 */
#ifndef SYSUTILS_H
#define SYSUTILS_H

#include <vector>
#include <cstdint>  // 添加这行以包含标准整数类型定义
#include <functional>// std::function

// #include "coincidenceanalyzer.h"
// 前向声明 DetTimeEnergy
// struct DetTimeEnergy;

//能谱多道的道数
constexpr unsigned short MULTI_CHANNEL = 8192;

enum Measure_status{
    msNone = 0x00,
    msEnd = 0x00, // 停止
    msPrepare = 0x01, // 准备
    msWaiting = 0x02, // 等待触发（自动测量有一个等待时间段）
    msStart = 0x03,// 开始，测量中
};

enum Measure_model{
    mmNone = 0x00,
    mmManual= 0x01, // 手动
    mmAuto = 0x02, // 自动
    mmDefine = 0x03, // 标定
};

#pragma pack(push, 1) // 设置1字节对齐

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

// 符合计数的结果
struct CoincidenceResult{
    unsigned time = 0; //时刻，单位:s。
    int ConCount_single = 0;//一秒内的单符合的个数
    int ConCount_multiple = 0;//一秒内的多符合的个数
    int CountRate1 = 0; //探测器1的计数率，是指在能窗内的计数率
    int CountRate2 = 0; //探测器2的计数率，是指在能窗内的计数率
    double DeathRatio1 = 0.0; //死时间率，单位：百分比
    double DeathRatio2 = 0.0; //死时间率，单位：百分比
};

struct CurrentPoint{
    unsigned int time; //时刻(单位s)，以FPGA时钟进行计时，给出当前的能谱对应的时刻，第1个谱对应time=1。
    int dataPoint1 = 0; //这次处理的探测器1数据点数
    int dataPoint2 = 0; //这次处理的探测器2数据点数
};

// 包大小65536 Byte
struct SingleSpectrum{ //存放每秒钟的能谱数据。1s产生一个能谱
    unsigned int time; //时刻(单位s)，以FPGA时钟进行计时，给出当前的能谱对应的时刻，第1个谱对应time=1。
    unsigned int spectrum[2][MULTI_CHANNEL] = {}; //探测器1能谱,全部初始化为0
};

struct AutoGaussFit{
    int time; //修改时间，单位s
    unsigned short EnLeft1;  // 探测器1符合能窗左边界
    unsigned short EnRight1; // 探测器1符合能窗右边界
    unsigned short EnLeft2;  // 探测器2符合能窗左边界
    unsigned short EnRight2; // 探测器2符合能窗右边界
};

typedef struct tagDetectorParameter{
    // 触发阈值
    int16_t triggerThold1, triggerThold2;

    // 波形极性
    /*
        00:正极性
        01:负极性
        默认正极性
    */
    int8_t waveformPolarity;


    // 能谱模式/粒子模式死时间 单位ns
    int16_t deadTime;
    // 能谱刷新时间
    int32_t refreshTimeLength;
    //波形长度
    int32_t waveLength;
    //波形触发模式
    int8_t triggerModel;

    // 探测器增益
    int8_t gain;

    // 传输模式
    /*
    00:能谱
    03:波形
    05:粒子模式
    */
    int8_t transferModel;

    // 测量模式
    int8_t measureModel;//01-手动测量 02-自动测量 03-标定测量

    //运行的量程
    int measureRange; // 量程1，量程2，量程3，量程4。。。

    //梯形成形
    bool isTrapShaping;
    //基线下限位置，基线是高斯噪声，这里是指基线下限，无梯形成形的时候必须是8140（十进制）；有梯形成形的时候默认值20（十进制）,可调节
    int32_t TrapShape_baseLine;
    //梯形成形上升沿点数，默认20
    int8_t TrapShape_risePoint;
    //梯形成形平顶点数，默认20
    int8_t TrapShape_peakPoint;
    //梯形成形下降沿点数，默认20
    int8_t TrapShape_fallPoint;
    //梯形成形时间常数t1 （乘以65536后取整）   默认为t1= 0.9558*65536；t2= 0.9556*65536；
    uint16_t TrapShape_constTime1;
    //梯形成形时间常数t2 （乘以65536后取整）   默认为t1= 0.9558*65536；t2= 0.9556*65536；
    uint16_t TrapShape_constTime2;

    int32_t coolingTime;//冷却时长,单位s,需要注意的是，对于自动测量与手动测量，这个冷却时间不一样
                        //手动测量与界面上冷却时间输入按钮不一致，自动测量与界面输入时间一致。
    int32_t delayTime;//延迟时间,单位ns
    int32_t timeWidth;//时间窗宽度，单位ns(符合分辨时间)
} DetectorParameter;
#pragma pack(pop) // 恢复默认对齐

using namespace std;
#define MAX_SPECTUM (MULTI_CHANNEL + 100)
#define PARTICLE_NUM_ONE_PAKAGE 64 //符合模式下，单个数据包的粒子数据个数

class SysUtils
{
public:
    SysUtils();

    static std::vector<DetTimeEnergy> getDetTimeEnergy(const char* filename);


    static void realAnalyzeTimeEnergy(const char* filename, std::function<void(DetTimeEnergy, bool/*结束标识*/, bool */*是否被终止*/)> callback);

    /*
     * 快速解析：要求文件除了裸数据，没有其他脏数据
    */
    static void realQuickAnalyzeTimeEnergy(const char* filename, std::function<void(DetTimeEnergy, bool/*结束标识*/, bool */*是否被终止*/)> callback);
};

#endif // SYSUTILS_H
