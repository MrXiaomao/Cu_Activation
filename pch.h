#ifndef PCH_H
#define PCH_H

//测量进度
enum Measurement_process{
    mpNone = 0x00,      //未开始
    mpStart,            //测试开始
    mpPrepare,          //准备阶段
    mpRunning,          //正式运行阶段
    mpEnd = mpNone,     //结束
};

#endif // PCH_H
