#include "controlhelper.h"
#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>

ControlHelper::ControlHelper(QObject *parent) : QObject(parent)
{
    this->load();
}

/// 获取sdk的版本号，如“1.1.0.0”
const char* ControlHelper::getsdkversion()
{
    return fti_getsdkversion();
}

/// 连接串口（Modbus-RTU），初始化，返回句柄。
bool ControlHelper::open_com(int axis_no)
{
    if (mHandle != 0){
        close();
        mHandle = 0;
    }

    QString com = "COM1";
    int baud = 19200;
    if (axis_no == 0x02)
        com = "COM2";

    int ret = fti_open_com(com.toStdString().c_str(), baud, FT_TRUE, &mHandle);
    return (ret == FT_SUCCESS);
}

/// 连接网络（Modbus-TCP），初始化，返回句柄。
bool ControlHelper::open_tcp()
{
    int ret = fti_open_tcp(mIp.toStdString().c_str(), mPort, FT_TRUE, &mHandle);    
    emit sigControlStatus(ret == FT_SUCCESS);
    return ret == FT_SUCCESS;
}

/// 关闭连接。
bool ControlHelper::close()
{
    int ret = fti_close(mHandle) == FT_SUCCESS;
    if (ret == FT_SUCCESS)
        emit sigControlStatus(false);

    return ret == FT_SUCCESS;
}

/// 修改驱动器地址。出厂默认地址为01。
bool ControlHelper::change_addr(int axis_no, ushort value)
{
    return fti_change_addr(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取电机型号，如“IM28”、“IM35”、“IM42”、“IM57ET_485”。
bool ControlHelper::get_motor_model(int axis_no, char* value)
{
    return fti_get_motor_model(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 永久保存当前参数，断电重启后保持。
bool ControlHelper::save_params_permanently(int axis_no)
{
    return fti_save_params_permanently(mHandle, mAxiaName[axis_no].toStdString().c_str()) == FT_SUCCESS;
}

/// 设置加速系数。
bool ControlHelper::set_accel(int axis_no, ushort value)
{
    return fti_set_accel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取加速系数。
bool ControlHelper::get_accel(int axis_no, ushort* value)
{
    return fti_get_accel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置减速系数。
bool ControlHelper::set_decel(int axis_no, ushort value)
{
    return fti_set_decel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取减速系数。
bool ControlHelper::get_decel(int axis_no, ushort* value)
{
    return fti_get_accel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置细分。
bool ControlHelper::set_div(int axis_no, int value)
{
    return fti_set_decel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取细分。
bool ControlHelper::get_div(int axis_no, int* value)
{
    return fti_get_div(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置螺距。
bool ControlHelper::set_pitch(int axis_no, float value)
{
    return fti_set_decel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取螺距。
bool ControlHelper::get_pitch(int axis_no, float* value)
{
    return fti_get_pitch(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置速度（运行速度）。
bool ControlHelper::set_vel(int axis_no, float value)
{
    return fti_set_vel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取速度（运行速度）。
bool ControlHelper::get_vel(int axis_no, float* value)
{
    return fti_get_vel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置软限位的位置下限。
bool ControlHelper::set_sw_p1(int axis_no, float value)
{
    return fti_set_sw_p1(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取软限位的位置下限。
bool ControlHelper::get_sw_p1(int axis_no, float* value)
{
    return fti_get_sw_p1(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置软限位的位置上限。
bool ControlHelper::set_sw_p2(int axis_no, float value)
{
    return fti_set_sw_p2(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取软限位的位置上限。
bool ControlHelper::get_sw_p2(int axis_no, float* value)
{
    return fti_get_sw_p2(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 单轴停止运动。
bool ControlHelper::single_stop(int axis_no)
{
    return fti_single_stop(mHandle, mAxiaName[axis_no].toStdString().c_str()) == FT_SUCCESS;
}

/// 单轴将当前位置设为零（若在运动则会先停止运动）。
bool ControlHelper::single_zero(int axis_no)
{
    return fti_single_zero(mHandle, mAxiaName[axis_no].toStdString().c_str()) == FT_SUCCESS;
}

/// 单轴搜零（触发内置搜零程序）。
bool ControlHelper::single_home(int axis_no)
{
    return fti_single_home(mHandle, mAxiaName[axis_no].toStdString().c_str()) == FT_SUCCESS;
}

/// 单轴相对运动。
bool ControlHelper::single_move(int axis_no, float value)
{
    return fti_single_move(mHandle, mAxiaName[axis_no].toStdString().c_str(), value * 1000) == FT_SUCCESS;
}

/// 单轴绝对运动。
bool ControlHelper::single_moveabs(int axis_no, float value)
{
    return fti_single_moveabs(mHandle, mAxiaName[axis_no].toStdString().c_str(), value * 1000) == FT_SUCCESS;
}

/// 单轴向左（负向）连续运动。
bool ControlHelper::single_jogleft(int axis_no)
{
    return fti_single_jogleft(mHandle, mAxiaName[axis_no].toStdString().c_str()) == FT_SUCCESS;
}

/// 单轴向右（正向）连续运动。
bool ControlHelper::single_jogright(int axis_no)
{
    return fti_single_jogright(mHandle, mAxiaName[axis_no].toStdString().c_str()) == FT_SUCCESS;
}

/// 单轴获取当前位值（绝对坐标值）。
bool ControlHelper::single_getpos(int axis_no, float* value)
{
    return fti_single_getpos(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 单轴获取当前状态，包括运行状态、输入口状态等，据此再判别是否运行（fti_single_isrunning）和正负限位状态（fti_single_getlimits）。
bool ControlHelper::single_getstatus(int axis_no, uint32_t* value)
{
    return fti_single_getstatus(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 单轴查询是否正在运动。
bool ControlHelper::single_isrunning(uint32_t status, byte* value)
{
    return fti_single_isrunning(status, value) == FT_SUCCESS;
}

/// 单轴获取限位状态。
bool ControlHelper::single_getlimits(uint32_t status, byte* values)
{
    return fti_single_getlimits(status, values) == FT_SUCCESS;
}

/// 使能或失能马达。
bool ControlHelper::single_setenabled(int axis_no, byte value)
{
    return fti_single_setenabled(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置寄存器的值，UINT32类型。
bool ControlHelper::set_uint32(int axis_no, const int reg_addr, uint32_t value)
{
    return fti_set_uint32(mHandle, mAxiaName[axis_no].toStdString().c_str(), reg_addr, value) == FT_SUCCESS;
}

/// 读取寄存器的值，UINT32类型。
bool ControlHelper::get_uint32(int axis_no, const int reg_addr, uint32_t* value)
{
    return fti_get_uint32(mHandle, mAxiaName[axis_no].toStdString().c_str(), reg_addr, value) == FT_SUCCESS;
}

/// 设置寄存器的值，UIN16类型。
bool ControlHelper::set_uint16(int axis_no, const int reg_addr, uint16_t value)
{
        return fti_set_uint16(mHandle, mAxiaName[axis_no].toStdString().c_str(), reg_addr, value) == FT_SUCCESS;
}

/// 读取寄存器的值，UIN16类型。
bool ControlHelper::get_uint16(int axis_no, const int reg_addr, uint16_t* value)
{
    return fti_get_uint16(mHandle, mAxiaName[axis_no].toStdString().c_str(), reg_addr, value) == FT_SUCCESS;
}

void ControlHelper::load()
{
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonObject jsonControl;

        if (jsonObj.contains("Control")){
            jsonControl = jsonObj["Control"].toObject();
            mIp = jsonControl["ip"].toString();
            mPort = jsonControl["port"].toInt();
        }
    }
}

void ControlHelper::gotoAbs(int index)
{
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        if (jsonObj.contains("Control")){
            QJsonObject jsonControl = jsonObj["Control"].toObject();
            QJsonArray jsonDistances = jsonControl["Distances"].toArray();

            if (jsonControl.contains("Distances")){
                QJsonObject jsonDistance1 = jsonControl["Distances"].toObject()["01"].toObject();
                QJsonObject jsonDistance2 = jsonControl["Distances"].toObject()["02"].toObject();
                {
                    //float speed;
                    //get_vel(0x01, &speed);
                    //set_vel(0x01, 5.0);
                    double pos = 0.;
                    switch (index){
                        case 0x00: pos = jsonDistance1["0-1E4"].toString().toDouble(); break;
                        case 0x01: pos = jsonDistance1["1E4-1E7"].toString().toDouble(); break;
                        case 0x02: pos = jsonDistance1["1E7-1E10"].toString().toDouble(); break;
                        case 0x03: pos = jsonDistance1["1E10-1E13"].toString().toDouble(); break;
                    }

                    single_moveabs(0x01, pos);
                    //set_vel(0x01, speed);
                }
                {
                    double pos = 0.;
                    switch (index){
                        case 0x00: pos = jsonDistance2["0-1E4"].toString().toDouble(); break;
                        case 0x01: pos = jsonDistance2["1E4-1E7"].toString().toDouble(); break;
                        case 0x02: pos = jsonDistance2["1E7-1E10"].toString().toDouble(); break;
                        case 0x03: pos = jsonDistance2["1E10-1E13"].toString().toDouble(); break;
                    }

                    single_moveabs(0x02, pos);
                }
            }
        }
    }
}
