#ifndef FTCOREIMC_H
#define FTCOREIMC_H

/* Add this for macros that defined unix flavor */
#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif

#ifndef _MSC_VER
#include <stdint.h>
#else
#include "stdint.h"
#endif

#if defined(_MSC_VER)
#if defined(DLLBUILD)
/* define DLLBUILD when building the DLL */
#define FTCORE_API __declspec(dllexport)
#else
#define FTCORE_API __declspec(dllimport)
#endif
#else
#define FTCORE_API
#endif

#ifdef __cplusplus
#define FTCORE_BEGIN_DECLS extern "C" {
#define FTCORE_END_DECLS }
#else
#define FTCORE_BEGIN_DECLS
#define FTCORE_END_DECLS
#endif

FTCORE_BEGIN_DECLS

#define SDK_VERSION "1.3.1.0"

// 64bit，用作句柄
typedef unsigned long long FT_H;

typedef unsigned char byte;
typedef unsigned short ushort;

typedef struct _ftcore ftcore_t;

// 注： bool 类型用 byte 实现，用宏定义 FT_TRUE / FT_FALSE
#define FT_TRUE 1
#define FT_FALSE 0

// 用于IO控制和输入口电平判断，高电平HIGH 或 低电平LOW
#define FT_IO_HIGH 1
#define FT_IO_LOW 0

// 错误码
#define FT_SUCCESS 0
#define FT_ERR_INVALID_HANDLE 0x8001
#define FT_ERR_INVALID_FEATURE 0x8002
#define FT_ERR_NOTIMPLEMENTED 0x8003
#define FT_ERR_INVALID_AXIS 0x8004

/// <summary>
/// 获取sdk的版本号，如“1.1.0.0”
/// </summary>
/// <returns>版本号字符串，如“1.1.0.0”</returns>
FTCORE_API const char* fti_getsdkversion();

/// <summary>
/// 连接串口（Modbus-RTU），初始化，返回句柄。
/// </summary>
/// <param
/// name="device">串口设备标识，windows系统下为“COM1”、“COM2”、“COM10”等，
///     Linux下为“/dev/ttyS0”、“/dev/ttyUSB0”等。</param>
/// <param name="baud">串口通信波特率，常用19200bps</param>
/// <param
/// name="limit_isnegative">限位的有效电平是否为负逻辑（即低电平表示触发），
///     否则为正逻辑（即高电平表示触发），FT_TRUE或FT_FALSE</param>
/// <param name="handle">【返回值】句柄</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_open_com(const char* device, const int baud,
                            const byte limit_isnegative, FT_H* handle);

/// <summary>
/// 连接网络（Modbus-TCP），初始化，返回句柄。
/// </summary>
/// <param name="ip">IP地址，如“192.168.1.120”</param>
/// <param name="port">端口号，如10001</param>
/// <param
/// name="limit_isnegative">限位的有效电平是否为负逻辑（即低电平表示触发），
///     否则为正逻辑（即高电平表示触发），FT_TRUE或FT_FALSE</param>
/// <param name="handle">【返回值】句柄</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_open_tcp(const char* ip, const int port,
                            const byte limit_isnegative, FT_H* handle);

/// <summary>
/// 关闭连接。
/// </summary>
/// <param name="handle">句柄</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_close(FT_H handle);

/// <summary>
/// 修改驱动器地址。出厂默认地址为01。
///
/// 特别说明：（1）调用该函数修改驱动器地址后，后续的通信均需要使用新地址，务必记录好新地址。
///         （2）实际地址=基地址+拨码地址-1，该函数将根据电机型号自动转换（不同电机的拨码地址不同），确保实际地址为欲设置的驱动器地址。
///         （3）底层通过设置基地址实现。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等表示驱动器的当前地址</param>
/// <param name="value">驱动器的新地址</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_change_addr(FT_H handle, const char* axis, ushort value);

/// <summary>
/// 获取电机型号，如“IM28”、“IM35”、“IM42”、“IM57ET_485”。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">【返回值】电机型号</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_get_motor_model(FT_H handle, const char* axis, char* value);

/// <summary>
/// 永久保存当前参数，断电重启后保持。
///
/// 特别说明：（1）带记忆寄存器的擦写寿命是10万次，请谨慎使用。建议不使用“自动保存参数”类似功能。
///         （2）部分参数需要调用该函数才能断电保存，否则断电后丢失，如螺距、闭环中的PID参数等。
///         （3）调用该函数时，所有带记忆寄存器都会保存。在未断电情况下，用户可以一次性把需要保存的参数设置好，再调用该函数。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_save_params_permanently(FT_H handle, const char* axis);

/// <summary>
/// 设置加速系数。
///
/// 特别说明：电机加速时间，从启动速度到目标速度需要的时间，单位ms。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">加速系数</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_set_accel(FT_H handle, const char* axis, ushort value);

/// <summary>
/// 获取加速系数。
///
/// 特别说明：电机加速时间，从启动速度到目标速度需要的时间，单位ms。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">【返回值】加速系数</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_get_accel(FT_H handle, const char* axis, ushort* value);

/// <summary>
/// 设置减速系数。
///
/// 特别说明：电机减速时间，从目标速度到停止速度需要的时间，单位ms。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">减速系数</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_set_decel(FT_H handle, const char* axis, ushort value);

/// <summary>
/// 获取减速系数。
///
/// 特别说明：电机减速时间，从目标速度到停止速度需要的时间，单位ms。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">【返回值】减速系数</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_get_decel(FT_H handle, const char* axis, ushort* value);

/// <summary>
/// 设置细分。
///
/// 特别说明：细分是指电机旋转一周所需的脉冲数。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">细分，常用1600</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_set_div(FT_H handle, const char* axis, int value);

/// <summary>
/// 获取细分。
///
/// 特别说明：细分是指电机旋转一周所需的脉冲数。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">【返回值】细分</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_get_div(FT_H handle, const char* axis, int* value);

/// <summary>
/// 设置螺距。
///
/// 特别说明：对于平移台，螺距是指电机旋转一周实际平移的距离。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">螺距，单位mm</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_set_pitch(FT_H handle, const char* axis, float value);

/// <summary>
/// 获取螺距。
///
/// 特别说明：对于平移台，螺距是指电机旋转一周实际平移的距离。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">【返回值】螺距，单位mm</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_get_pitch(FT_H handle, const char* axis, float* value);

/// <summary>
/// 设置速度（运行速度）。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">速度</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_set_vel(FT_H handle, const char* axis, float value);

/// <summary>
/// 获取速度（运行速度）。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">【返回值】速度</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_get_vel(FT_H handle, const char* axis, float* value);

/// <summary>
/// 设置软限位的位置下限。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">软限位的位置下限</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_set_sw_p1(FT_H handle, const char* axis, float value);

/// <summary>
/// 获取软限位的位置下限。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">【返回值】软限位的位置下限</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_get_sw_p1(FT_H handle, const char* axis, float* value);

/// <summary>
/// 设置软限位的位置上限。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">软限位的位置上限</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_set_sw_p2(FT_H handle, const char* axis, float value);

/// <summary>
/// 获取软限位的位置上限。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">【返回值】软限位的位置上限</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_get_sw_p2(FT_H handle, const char* axis, float* value);

/// <summary>
/// 单轴停止运动。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_single_stop(FT_H handle, const char* axis);

/// <summary>
/// 单轴将当前位置设为零（若在运动则会先停止运动）。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_single_zero(FT_H handle, const char* axis);

/// <summary>
/// 单轴搜零（触发内置搜零程序）。
/// </summary>
/// <param name="handle">句柄</param>
/// <param
/// name="axis">轴号，AMC的轴号为XYZUVWAB之一，NANO和MINI04则用“01”、“02”、“03”等地址表示</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_single_home(FT_H handle, const char* axis);

/// <summary>
/// 单轴相对运动。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">相对运动距离</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_single_move(FT_H handle, const char* axis, float value);

/// <summary>
/// 单轴绝对运动。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">绝对位置坐标</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_single_moveabs(FT_H handle, const char* axis, float value);

/// <summary>
/// 单轴向左（负向）连续运动。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_single_jogleft(FT_H handle, const char* axis);

/// <summary>
/// 单轴向右（正向）连续运动。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_single_jogright(FT_H handle, const char* axis);

/// <summary>
/// 单轴获取当前位值（绝对坐标值）。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">【返回值】当前位置</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_single_getpos(FT_H handle, const char* axis, float* value);

/// <summary>
/// 单轴获取当前状态，包括运行状态、输入口状态等，据此再判别是否运行（fti_single_isrunning）和正负限位状态（fti_single_getlimits）。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">【返回值】当前状态，包括运行状态和输入口状态等</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_single_getstatus(FT_H handle, const char* axis,
                                    uint32_t* value);

/// <summary>
/// 单轴查询是否正在运动。
///
/// 特别说明：从状态值中进行解析，状态值通过fti_single_getstatus获得。
/// </summary>
/// <param name="status">状态值</param>
/// <param name="value">【返回值】运行中FT_TRUE，停止FT_FALSE</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_single_isrunning(uint32_t status, byte* value);

/// <summary>
/// 单轴获取限位状态。
///
/// 特别说明：从状态值中进行解析，状态值通过fti_single_getstatus获得。
/// </summary>
/// <param name="status">状态值</param>
/// <param
/// name="values">【返回值，数组】触发FT_TRUE、未触发FT_FALSE，依次为负限位、正限位</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_single_getlimits(uint32_t status, byte* values);

/// <summary>
/// 使能或失能马达。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="value">FT_TRUE使能，FT_FALSE失能</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_single_setenabled(FT_H handle, const char* axis, byte value);

/// <summary>
/// 设置寄存器的值，UINT32类型。
///
/// 特别说明：请谨慎使用，确保地址正确、类型正确。
///         若实际值为负数，则强制类型转换为uint32类型。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="reg_addr">寄存器地址</param>
/// <param name="value">数值，UINT32</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_set_uint32(FT_H handle, const char* axis, const int reg_addr,
                              uint32_t value);

/// <summary>
/// 读取寄存器的值，UINT32类型。
///
/// 特别说明：请谨慎使用，确保地址正确、类型正确。
///         若实际值为负数，则强制类型转换为int32类型。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="reg_addr">寄存器地址</param>
/// <param name="value">【返回值】当前数值</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_get_uint32(FT_H handle, const char* axis, const int reg_addr,
                              uint32_t* value);

/// <summary>
/// 设置寄存器的值，UIN16类型。
///
/// 特别说明：请谨慎使用，确保地址正确、类型正确。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="reg_addr">寄存器地址</param>
/// <param name="value">数值，UINT16</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_set_uint16(FT_H handle, const char* axis, const int reg_addr,
                              uint16_t value);

/// <summary>
/// 读取寄存器的值，UIN16类型。
///
/// 特别说明：请谨慎使用，确保地址正确、类型正确。
/// </summary>
/// <param name="handle">句柄</param>
/// <param name="axis">轴号，用“01”、“02”、“03”等地址表示</param>
/// <param name="reg_addr">寄存器地址</param>
/// <param name="value">【返回值】当前数值，UINT16</param>
/// <returns>错误代码，FT_SUCESS（0）表示成功</returns>
FTCORE_API int fti_get_uint16(FT_H handle, const char* axis, const int reg_addr,
                              uint16_t* value);

FTCORE_END_DECLS

#endif /* FTCOREIMC_H */
