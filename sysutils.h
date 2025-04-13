#ifndef SYSUTILS_H
#define SYSUTILS_H

#include <vector>
#include "coincidenceanalyzer.h"

using namespace std;

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

    int32_t cool_timelength;//冷却时长
} DetectorParameter;

class SysUtils
{
public:
    SysUtils();

    static std::vector<DetTimeEnergy> getDetTimeEnergy(const char* filename);

    static void realAnalyzeTimeEnergy(const char* filename, std::function<void(DetTimeEnergy)> callback);
};

#endif // SYSUTILS_H
