#ifndef CONTROLHELPER_H
#define CONTROLHELPER_H

#include <QObject>
#include <QMutex>
#include "ftcoreimc.h"
#include "quithread.h"

class ControlHelper : public QObject
{
    Q_OBJECT
public:
    explicit ControlHelper(QObject *parent = nullptr);
    ~ControlHelper();

    static ControlHelper *instance() {
        static ControlHelper controlHelper;
        return &controlHelper;
    }

    void load();
    bool connected();

    void gotoAbs(int index, float max_speed = 5.);

    /// 获取sdk的版本号，如“1.1.0.0”
    const char* getsdkversion();

    /// 连接串口（Modbus-RTU），初始化，返回句柄。
    bool open_com(int axis_no);

    /// 连接网络（Modbus-TCP），初始化，返回句柄。
    bool open_tcp();

    /// 关闭连接。
    bool close();

    /// 修改驱动器地址。出厂默认地址为01。
    bool change_addr(int axis_no, ushort value);

    /// 获取电机型号，如“IM28”、“IM35”、“IM42”、“IM57ET_485”。
    bool get_motor_model(int axis_no, char* value);

    /// 永久保存当前参数，断电重启后保持。
    bool save_params_permanently(int axis_no);

    /// 设置加速系数。
    bool set_accel(int axis_no, ushort value);

    /// 获取加速系数。
    bool get_accel(int axis_no, ushort* value);

    /// 设置减速系数。
    bool set_decel(int axis_no, ushort value);

    /// 获取减速系数。
    bool get_decel(int axis_no, ushort* value);

    /// 设置细分。
    bool set_div(int axis_no, int value);

    /// 获取细分。
    bool get_div(int axis_no, int* value);

    /// 设置螺距。
    bool set_pitch(int axis_no, float value);

    /// 获取螺距。
    bool get_pitch(int axis_no, float* value);

    /// 设置速度（运行速度）。
    bool set_vel(int axis_no, float value);

    /// 获取速度（运行速度）。
    bool get_vel(int axis_no, float* value);

    /// 设置软限位的位置下限。
    bool set_sw_p1(int axis_no, float value);

    /// 获取软限位的位置下限。
    bool get_sw_p1(int axis_no, float* value);

    /// 设置软限位的位置上限。
    bool set_sw_p2(int axis_no, float value);

    /// 获取软限位的位置上限。
    bool get_sw_p2(int axis_no, float* value);

    /// 单轴停止运动。
    bool single_stop(int axis_no);

    /// 单轴将当前位置设为零（若在运动则会先停止运动）。
    bool single_zero(int axis_no);

    /// 单轴搜零（触发内置搜零程序）。
    bool single_home(int axis_no);

    /// 单轴相对运动。
    bool single_move(int axis_no, float value);

    /// 单轴绝对运动。
    bool single_moveabs(int axis_no, float value);

    /// 单轴向左（负向）连续运动。
    bool single_jogleft(int axis_no);

    /// 单轴向右（正向）连续运动。
    bool single_jogright(int axis_no);

    /// 单轴获取当前位值（绝对坐标值）。
    bool single_getpos(int axis_no, float* value);

    /// 单轴获取当前状态，包括运行状态、输入口状态等，据此再判别是否运行（fti_single_isrunning）和正负限位状态（fti_single_getlimits）。
    bool single_getstatus(int axis_no, uint32_t* value);

    /// 单轴查询是否正在运动。
    bool single_isrunning(uint32_t status, byte* value);

    /// 单轴获取限位状态。
    bool single_getlimits(uint32_t status, byte* values);

    /// 使能或失能马达。
    bool single_setenabled(int axis_no, byte value);

    /// 设置寄存器的值，UINT32类型。
    bool set_uint32(int axis_no, const int reg_addr,
                                  uint32_t value);

    /// 读取寄存器的值，UINT32类型。
    bool get_uint32(int axis_no, const int reg_addr,
                                  uint32_t* value);

    /// 设置寄存器的值，UIN16类型。
    bool set_uint16(int axis_no, const int reg_addr,
                                  uint16_t value);

    /// 读取寄存器的值，UIN16类型。
    bool get_uint16(int axis_no, const int reg_addr,
                                  uint16_t* value);

signals:
    // 位移平台状态
    void sigControlStatus(bool on);
    void sigControlFault(qint32 index);//故障，一般指网络不通
    void sigReportAbs(float f1, float f2);
    void sigReportStatus(int i, bool l, bool r, bool s);
    void sigError(QString);

private:
    QMutex mutex;
    FT_H mHandle;    
    bool mConnected = false;
    QString mIp;
    qint32 mPort;
    QString mAxiaName[3] = {"'",  "01", "02" };
    QUiThread* mpWorkThread;
    bool taskFinished = false;
};

#endif // CONTROLHELPER_H
