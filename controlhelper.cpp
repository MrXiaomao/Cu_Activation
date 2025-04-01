#include "controlhelper.h"
#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QTimer>

//#include <QLibrary>
//extern "C" {
//    typedef __cdecl  int (*PFfti_open_tcp)(const char* ip, const int port, const byte limit_isnegative, FT_H* handle);
//    typedef const char* (*PFfti_getsdkversion)();
//}

ControlHelper::ControlHelper(QObject *parent) : QObject(parent)
{
    this->load();

    mpWorkThread = new QUiThread(this);
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
                    sigReportAbs(pos1, pos2);
                }

                ret = single_getstatus(0x01, &status);
                if (ret){
                    single_isrunning(status, &isrunning);
                    single_getlimits(status, limits);

                    sigReportStatus(0x01, limits[0] == FT_TRUE, limits[1] == FT_TRUE, isrunning == FT_TRUE);
                }

                ret = single_getstatus(0x02, &status);
                if (ret){
                    single_isrunning(status, &isrunning);
                    single_getlimits(status, limits);

                    sigReportStatus(0x02, limits[0] == FT_TRUE, limits[1] == FT_TRUE, isrunning == FT_TRUE);
                }
            }

            QThread::msleep(100);
        }
    });
    mpWorkThread->start();
    connect(this, &QUiThread::destroyed, [=]() {
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
    int ret = fti_open_tcp(mIp.toStdString().c_str(), mPort, FT_TRUE, &mHandle);
    mConnected = ret == FT_SUCCESS;
    emit sigControlStatus(mConnected);

//细分 螺距 加速系数 减速系数
//            fti_get_div(mCurrentHandle, currentAxiaName.toStdString().c_str(), &iValue);
//            fti_get_pitch(mCurrentHandle, currentAxiaName.toStdString().c_str(), &dValue);
//            fti_get_accel(mCurrentHandle, currentAxiaName.toStdString().c_str(), &uValue);
//            fti_get_decel(mCurrentHandle, currentAxiaName.toStdString().c_str(), &uValue);
    return ret == FT_SUCCESS;
}

/// 关闭连接。
bool ControlHelper::close()
{
    if (mHandle == 0)
        return false;

    QMutexLocker locker(&mutex);
    int ret = fti_close(mHandle) == FT_SUCCESS;
    if (ret != FT_SUCCESS){
        int errRef = 1;
        while (errRef < 3){
            ret = fti_close(mHandle);
            if (ret != FT_SUCCESS)
                ++errRef;
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

    return fti_single_home(mHandle, mAxiaName[axis_no].toStdString().c_str()) == FT_SUCCESS;
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

    int ret = fti_single_moveabs(mHandle, mAxiaName[axis_no].toStdString().c_str(), value);
    if (ret != FT_SUCCESS){
        int errRef = 1;
        while (errRef < 3){
            ret = fti_single_moveabs(mHandle, mAxiaName[axis_no].toStdString().c_str(), value);
            if (ret != FT_SUCCESS)
                ++errRef;
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

/// 单轴获取当前位值（绝对坐标值）。
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

void ControlHelper::gotoAbs(int index, float max_speed)
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
//                    float speed;
//                    get_vel(0x01, &speed);
//                    set_vel(0x01, max_speed);
                    double pos = 0.;
                    switch (index){
                        case 0x00: pos = jsonDistance1["0-1E4"].toString().toDouble(); break;
                        case 0x01: pos = jsonDistance1["1E4-1E7"].toString().toDouble(); break;
                        case 0x02: pos = jsonDistance1["1E7-1E10"].toString().toDouble(); break;
                        case 0x03: pos = jsonDistance1["1E10-1E13"].toString().toDouble(); break;
                    }

                    single_moveabs(0x01, pos * 1000);
//                    QTimer::singleShot(5000, this, [=](){
//                        set_vel(0x01, speed);
//                    });
                }
                {
//                    float speed;
//                    get_vel(0x01, &speed);
//                    set_vel(0x01, max_speed);
                    double pos = 0.;
                    switch (index){
                        case 0x00: pos = jsonDistance2["0-1E4"].toString().toDouble(); break;
                        case 0x01: pos = jsonDistance2["1E4-1E7"].toString().toDouble(); break;
                        case 0x02: pos = jsonDistance2["1E7-1E10"].toString().toDouble(); break;
                        case 0x03: pos = jsonDistance2["1E10-1E13"].toString().toDouble(); break;
                    }

                    single_moveabs(0x02, pos * 1000);
//                    QTimer::singleShot(5000, this, [=](){
//                        set_vel(0x01, speed);
//                    });
                }
            }
        }
    }
}
