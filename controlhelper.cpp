#include "controlhelper.h"
#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QDebug>

//#include <QLibrary>
//extern "C" {
//    typedef __cdecl  int (*PFfti_open_tcp)(const char* ip, const int port, const byte limit_isnegative, FT_H* handle);
//    typedef const char* (*PFfti_getsdkversion)();
//}

ControlHelper::ControlHelper(QObject *parent) : QObject(parent)
{
    this->load();

    mpWorkThread = new QLiteThread(this);
    mpWorkThread->setObjectName("mpWorkThread");
    mpWorkThread->setWorkThreadProc([=](){
        while (!taskFinished) {
            {
                QMutexLocker locker(&mutex);

                float pos1, pos2;
                uint32_t status;
                byte isrunning;
                byte limits[2];
                bool ret = single_getpos(0x01, &pos1);
                if (single_getpos(0x01, &pos1) && single_getpos(0x02, &pos2)){
                    emit sigReportAbs(pos1, pos2);
                }

                ret = single_getstatus(0x01, &status);
                if (ret){
                    single_isrunning(status, &isrunning);
//                    if (!isrunning && this->property("single-home").isValid()){
//                        this->setProperty("axis01-single-home", false);//搜零完毕
//                    }

                    single_getlimits(status, limits);
                    emit sigReportStatus(0x01, limits[0] == FT_TRUE, limits[1] == FT_TRUE, isrunning == FT_TRUE);
                }

                ret = single_getstatus(0x02, &status);
                if (ret){
                    single_isrunning(status, &isrunning);
//                    if (!isrunning && this->property("single-home").isValid()){
//                        this->setProperty("axis01-single-home", false);//搜零完毕
//                    }

                    single_getlimits(status, limits);

                    emit sigReportStatus(0x02, limits[0] == FT_TRUE, limits[1] == FT_TRUE, isrunning == FT_TRUE);
                }
            }

            QThread::msleep(200);
        }
    });
    mpWorkThread->start();
    connect(this, &QLiteThread::destroyed, [=]() {
        mpWorkThread->exit(0);
        mpWorkThread->wait(500);
    });
}

ControlHelper::~ControlHelper(){
    taskFinished = true;
    mpWorkThread->wait();
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

bool ControlHelper::connected()
{
    return mConnected;
}

/// 连接网络（Modbus-TCP），初始化，返回句柄。
bool ControlHelper::open_tcp()
{
    load();
    int ret = fti_open_tcp(mIp.toStdString().c_str(), mPort, FT_TRUE, &mHandle);
    mConnected = ret == FT_SUCCESS;
    emit sigControlStatus(mConnected);

//细分 螺距 加速系数 减速系数
//            fti_get_div(mCurrentHandle, currentAxiaName.toStdString().c_str(), &iValue);
//            fti_get_pitch(mCurrentHandle, currentAxiaName.toStdString().c_str(), &dValue);
//            fti_get_accel(mCurrentHandle, currentAxiaName.toStdString().c_str(), &uValue);
//            fti_get_decel(mCurrentHandle, currentAxiaName.toStdString().c_str(), &uValue);

    if (mConnected){
        //single_home(0x01);

        //single_home(0x02);

//        QPair<float, float> pair = gotoAbs(0);
//        this->setProperty("axis01-target-position", pair.first);
//        this->setProperty("axis02-target-position", pair.second);
    }

    return ret == FT_SUCCESS;
}

/// 关闭连接。
bool ControlHelper::close()
{
    if (mHandle == 0)
        return false;

    QMutexLocker locker(&mutex);
    int ret = fti_close(mHandle);
    if (ret != FT_SUCCESS){
        int errRef = 1;
        while (errRef < 3){
            ret = fti_close(mHandle);
            if (ret != FT_SUCCESS)
                ++errRef;
            else
                break;
        }
        emit sigError(tr("位移平台断开连接失败，计数：%2").arg(errRef));
    }

    if (ret == FT_SUCCESS){
        mHandle = 0;
        emit sigControlStatus(false);
    }

    return ret == FT_SUCCESS;
}

/// 修改驱动器地址。出厂默认地址为01。
bool ControlHelper::change_addr(int axis_no, ushort value)
{
    if (mHandle == 0)
        return false;

    return fti_change_addr(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取电机型号，如“IM28”、“IM35”、“IM42”、“IM57ET_485”。
bool ControlHelper::get_motor_model(int axis_no, char* value)
{
    if (mHandle == 0)
        return false;

    return fti_get_motor_model(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 永久保存当前参数，断电重启后保持。
bool ControlHelper::save_params_permanently(int axis_no)
{
    if (mHandle == 0)
        return false;

    return fti_save_params_permanently(mHandle, mAxiaName[axis_no].toStdString().c_str()) == FT_SUCCESS;
}

/// 设置加速系数。
bool ControlHelper::set_accel(int axis_no, ushort value)
{
    if (mHandle == 0)
        return false;

    return fti_set_accel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取加速系数。
bool ControlHelper::get_accel(int axis_no, ushort* value)
{
    if (mHandle == 0)
        return false;

    return fti_get_accel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置减速系数。
bool ControlHelper::set_decel(int axis_no, ushort value)
{
    if (mHandle == 0)
        return false;

    return fti_set_decel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取减速系数。
bool ControlHelper::get_decel(int axis_no, ushort* value)
{
    if (mHandle == 0)
        return false;

    return fti_get_accel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置细分。
bool ControlHelper::set_div(int axis_no, int value)
{
    if (mHandle == 0)
        return false;

    return fti_set_decel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取细分。
bool ControlHelper::get_div(int axis_no, int* value)
{
    if (mHandle == 0)
        return false;

    return fti_get_div(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置螺距。
bool ControlHelper::set_pitch(int axis_no, float value)
{
    if (mHandle == 0)
        return false;

    return fti_set_decel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取螺距。
bool ControlHelper::get_pitch(int axis_no, float* value)
{
    if (mHandle == 0)
        return false;

    return fti_get_pitch(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置速度（运行速度）。
#include <QtMath>
bool ControlHelper::set_vel(int axis_no, float value)
{
    if (mHandle == 0)
        return false;

    return fti_set_vel(mHandle, mAxiaName[axis_no].toStdString().c_str(), qMin<double>(value, 5)) == FT_SUCCESS;
}

/// 获取速度（运行速度）。
bool ControlHelper::get_vel(int axis_no, float* value)
{
    if (mHandle == 0)
        return false;

    return fti_get_vel(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置软限位的位置下限。
bool ControlHelper::set_sw_p1(int axis_no, float value)
{
    if (mHandle == 0)
        return false;

    return fti_set_sw_p1(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取软限位的位置下限。
bool ControlHelper::get_sw_p1(int axis_no, float* value)
{
    if (mHandle == 0)
        return false;

    return fti_get_sw_p1(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置软限位的位置上限。
bool ControlHelper::set_sw_p2(int axis_no, float value)
{
    if (mHandle == 0)
        return false;

    return fti_set_sw_p2(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 获取软限位的位置上限。
bool ControlHelper::get_sw_p2(int axis_no, float* value)
{
    if (mHandle == 0)
        return false;

    return fti_get_sw_p2(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 单轴停止运动。
bool ControlHelper::single_stop(int axis_no)
{
    if (mHandle == 0)
        return false;

    int ret = fti_single_stop(mHandle, mAxiaName[axis_no].toStdString().c_str());
    if (ret != FT_SUCCESS){
        int errRef = 1;
        while (errRef < 3){
            ret = fti_single_stop(mHandle, mAxiaName[axis_no].toStdString().c_str());
            if (ret != FT_SUCCESS)
                ++errRef;
            else
                break;
        }
        emit sigError(tr("轴[%1]停止运行失败，计数：%2").arg(axis_no).arg(errRef));
    }

    return (ret == FT_SUCCESS);
}

/// 单轴将当前位置设为零（若在运动则会先停止运动）。
bool ControlHelper::single_zero(int axis_no)
{
    if (mHandle == 0)
        return false;

    return fti_single_zero(mHandle, mAxiaName[axis_no].toStdString().c_str()) == FT_SUCCESS;
}

/// 单轴搜零（触发内置搜零程序）。
bool ControlHelper::single_home(int axis_no)
{
    if (mHandle == 0)
        return false;

    int ret = fti_single_home(mHandle, mAxiaName[axis_no].toStdString().c_str());
    if (ret == FT_SUCCESS){
        this->setProperty("single-home", true);//正在搜零，搜零完成才能继续其他操作
    }

    return ret == FT_SUCCESS;
}

/// 单轴相对运动。
bool ControlHelper::single_move(int axis_no, float value)
{
    if (mHandle == 0)
        return false;

    return fti_single_move(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 单轴绝对运动。
bool ControlHelper::single_moveabs(int axis_no, float value)
{
    if (mHandle == 0)
        return false;

    QMutexLocker locker(&mutex);
    qDebug().noquote() << tr("轴") << axis_no << tr("(um))") << value;
    int ret = fti_single_moveabs(mHandle, mAxiaName[axis_no].toStdString().c_str(), value);
    if (ret != FT_SUCCESS){
        int errRef = 1;
        while (errRef < 3){
            ret = fti_single_moveabs(mHandle, mAxiaName[axis_no].toStdString().c_str(), value);
            if (ret != FT_SUCCESS)
                ++errRef;
            else
                break;
        }
        emit sigError(tr("轴[%1]移动到位置[%2]失败，计数：%2").arg(axis_no).arg(errRef));
    }

    return (ret == FT_SUCCESS);
}

/// 单轴向左（负向）连续运动。
bool ControlHelper::single_jogleft(int axis_no)
{
    if (mHandle == 0)
        return false;

    return fti_single_jogleft(mHandle, mAxiaName[axis_no].toStdString().c_str()) == FT_SUCCESS;
}

/// 单轴向右（正向）连续运动。
bool ControlHelper::single_jogright(int axis_no)
{
    if (mHandle == 0)
        return false;

    return fti_single_jogright(mHandle, mAxiaName[axis_no].toStdString().c_str()) == FT_SUCCESS;
}

/// 单轴获取当前位置（绝对坐标值）。
bool ControlHelper::single_getpos(int axis_no, float* value)
{
    if (mHandle == 0)
        return false;

    return fti_single_getpos(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 单轴获取当前状态，包括运行状态、输入口状态等，据此再判别是否运行（fti_single_isrunning）和正负限位状态（fti_single_getlimits）。
bool ControlHelper::single_getstatus(int axis_no, uint32_t* value)
{
    if (mHandle == 0)
        return false;

    return fti_single_getstatus(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 单轴查询是否正在运动。
bool ControlHelper::single_isrunning(uint32_t status, byte* value)
{
    if (mHandle == 0)
        return false;

    return fti_single_isrunning(status, value) == FT_SUCCESS;
}

/// 单轴获取限位状态。
bool ControlHelper::single_getlimits(uint32_t status, byte* values)
{
    if (mHandle == 0)
        return false;

    return fti_single_getlimits(status, values) == FT_SUCCESS;
}

/// 使能或失能马达。
bool ControlHelper::single_setenabled(int axis_no, byte value)
{
    if (mHandle == 0)
        return false;

    return fti_single_setenabled(mHandle, mAxiaName[axis_no].toStdString().c_str(), value) == FT_SUCCESS;
}

/// 设置寄存器的值，UINT32类型。
bool ControlHelper::set_uint32(int axis_no, const int reg_addr, uint32_t value)
{
    if (mHandle == 0)
        return false;

    return fti_set_uint32(mHandle, mAxiaName[axis_no].toStdString().c_str(), reg_addr, value) == FT_SUCCESS;
}

/// 读取寄存器的值，UINT32类型。
bool ControlHelper::get_uint32(int axis_no, const int reg_addr, uint32_t* value)
{
    if (mHandle == 0)
        return false;

    return fti_get_uint32(mHandle, mAxiaName[axis_no].toStdString().c_str(), reg_addr, value) == FT_SUCCESS;
}

/// 设置寄存器的值，UIN16类型。
bool ControlHelper::set_uint16(int axis_no, const int reg_addr, uint16_t value)
{
    if (mHandle == 0)
        return false;

    return fti_set_uint16(mHandle, mAxiaName[axis_no].toStdString().c_str(), reg_addr, value) == FT_SUCCESS;
}

/// 读取寄存器的值，UIN16类型。
bool ControlHelper::get_uint16(int axis_no, const int reg_addr, uint16_t* value)
{
    if (mHandle == 0)
        return false;

    return fti_get_uint16(mHandle, mAxiaName[axis_no].toStdString().c_str(), reg_addr, value) == FT_SUCCESS;
}

#include "globalsettings.h"
void ControlHelper::load()
{
    JsonSettings* ipSettings = GlobalSettings::instance()->mIpSettings;
    ipSettings->prepare();
    ipSettings->beginGroup("Control");
    mIp = ipSettings->value("ip", "192.168.10.3").toString();
    mPort = ipSettings->value("port", 5000).toInt();
    ipSettings->endGroup();
    ipSettings->finish();
}

QPair<float, float> ControlHelper::gotoAbs(int index, float max_speed)
{
    float pos1 = 0.0, pos2 = 0.0;

    JsonSettings* ipSettings = GlobalSettings::instance()->mIpSettings;
    ipSettings->prepare();
    ipSettings->beginGroup("Control");

    {
        switch (index){
        case 0x00: pos1 = ipSettings->childValue("Distances","01","smallRange").toString().toDouble();break;
        case 0x01: pos1 = ipSettings->childValue("Distances","01","mediumRange").toString().toDouble();break;
        case 0x02: pos1 = ipSettings->childValue("Distances","01","largeRange").toString().toDouble();break;
        }

        single_moveabs(0x01, pos1 * 1000);
    }

    {
        switch (index){
        case 0x00: pos2 = ipSettings->childValue("Distances","02","smallRange").toString().toDouble();break;
        case 0x01: pos2 = ipSettings->childValue("Distances","02","mediumRange").toString().toDouble();break;
        case 0x02: pos2 = ipSettings->childValue("Distances","02","largeRange").toString().toDouble();break;
        }

        single_moveabs(0x02, pos2 * 1000);
    }

    ipSettings->endGroup();
    ipSettings->finish();
    return QPair<float, float>(pos1, pos2);
}
