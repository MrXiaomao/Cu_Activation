/*
 * @Author: MaoXiaoqing
 * @Date: 2025-04-06 20:15:30
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2025-04-26 20:01:11
 * @Description: 请填写简介
 */
#ifndef SYSUTILS_H
#define SYSUTILS_H

#include <vector>
#include "coincidenceanalyzer.h"

using namespace std;
#define MAX_SPECTUM (MULTI_CHANNEL + 100)
#define PARTICLE_NUM_ONE_PAKAGE 64 //符合模式下，单个数据包的粒子数据个数
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

    int32_t cool_timelength;//冷却时长
} DetectorParameter;

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
