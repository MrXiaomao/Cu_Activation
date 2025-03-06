#include "commandhelper.h"
#include <QDataStream>

CommandHelper::CommandHelper(QObject *parent) : QObject(parent)
{
    socket = new QTcpSocket(this);
    int bufferSize = 4 * 1024 * 1024;
    socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, bufferSize);
    socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, bufferSize);
}

void CommandHelper::openPower(QString ip, qint32 port)
{
    //断开网络连接
    if (socket->isOpen())
        socket->disconnectFromHost();

    disconnect(socket, nullptr, this, nullptr);// 断开所有槽函数

    //网络异常
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [=](QAbstractSocket::SocketError){
        emit sigPowerStatus(false);
    });

    //状态改变
    connect(socket, &QAbstractSocket::stateChanged, this, [=](QAbstractSocket::SocketState){
    });

    //连接成功
    connect(socket, &QTcpSocket::connected, this, [=](){
        emit sigPowerStatus(true);

        //发送指令
        socket->write("read");
    });

    //接收数据
    connect(socket, &QTcpSocket::readyRead, this, [=](){
        QByteArray binaryData = socket->readAll();

        QString ack = QString::fromStdString(binaryData.toStdString());
        if (ack.startsWith("relay")){
            QString status = ack.remove("relay");
            bool b1 = status[0] == '1';
            bool b2 = status[1] == '1';
            emit sigRelayStatus(b1, b2);
        }
    });

    socket->connectToHost(ip, port);
}

void CommandHelper::closePower(QString ip, qint32 port)
{
    //断开网络连接
    if (socket->isOpen())
        socket->disconnectFromHost();

    disconnect(socket);// 断开所有槽函数
    emit sigPowerStatus(false);
    return;

    //网络异常
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [=](QAbstractSocket::SocketError){
        emit sigPowerStatus(false);
    });

    //状态改变
    connect(socket, &QAbstractSocket::stateChanged, this, [=](QAbstractSocket::SocketState){
    });

    //连接成功
    connect(socket, &QTcpSocket::connected, this, [=](){
        emit sigPowerStatus(true);

        //发送指令
        socket->write("close");
    });

    //接收数据
    connect(socket, &QTcpSocket::readyRead, this, [=](){
        QByteArray binaryData = socket->readAll();

        QString ack = QString::fromStdString(binaryData.toStdString());
        if (ack.startsWith("relay")){
            QString status = ack.remove("relay");
            bool b1 = status[0] == '1';
            bool b2 = status[1] == '1';
            emit sigRelayStatus(b1, b2);
        }
    });

    socket->connectToHost(ip, port);
}

void CommandHelper::makeFrame()
{
    //清空
    frame.clear();

    //帧头
    quint8 fs = 0x55;
    quint16 fe = 0xFE;
    quint16 fl = 0x0000;//帧长度
    quint32 dsn = 0x00000000;
    quint8 pid = 0x00;
    quint8 mid = 0x00;
    quint16 ml = 0x0000;

    QDataStream dataStream(&frame, QIODevice::WriteOnly);
    dataStream << (quint8)fs;//FS
    dataStream << (quint16)fl;//FL
    dataStream << (quint32)dsn;//DSN
    dataStream << (quint8)pid;//PID
    dataStream << (quint8)mid;//MID
    dataStream << (quint16)ml;//ML
    dataStream << data;//DATA
    dataStream << (quint8)crc();
    dataStream << (quint8)fe;
}

quint8 CommandHelper::crc()
{
    return 0x00;
}

//触发阈值1
void CommandHelper::slotTriggerThold1(quint16 ch1, quint16 ch2)
{
    //清空
    data.clear();

    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfe;
    dataStream << (quint8)0x11;

    dataStream << (quint16)ch2;
    dataStream << (quint16)ch1;

    dataStream << (quint16)0xab;
    dataStream << (quint16)0xcd;
}

//触发阈值2
void CommandHelper::slotTriggerThold2(quint16 ch3, quint16 ch4)
{
    //清空
    data.clear();

    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfe;
    dataStream << (quint8)0x12;

    dataStream << (quint16)ch3;
    dataStream << (quint16)ch4;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;
}

//波形极性
void CommandHelper::slotWaveformPolarity(quint8 v)
{
    //清空
    data.clear();

    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfe;
    dataStream << (quint8)0x13;

    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)v;//00:正极性 01:负极性

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;
}


//波形触发模式
void CommandHelper::slotWaveformTriggerMode(quint8 mode)
{
    //清空
    data.clear();

    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfe;
    dataStream << (quint8)0x14;

    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)mode;//00:normal 01:auto

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;
}

//波形长度
void CommandHelper::slotWaveformLength(quint8 v)
{
    //清空
    data.clear();

    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfe;
    dataStream << (quint8)0x15;

    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;

    /*
    00:64
    01:128
    02:256
    03:512
    04:1024
    默认1024
    */
    dataStream << (quint8)v;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;
}

//能谱模式/粒子模式死时间
void CommandHelper::slotSpectnumMode(quint16 dieTimelength)
{
    //清空
    data.clear();

    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfe;
    dataStream << (quint8)0x16;

    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint16)dieTimelength;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;
}

//能谱刷新时间
void CommandHelper::slotSpectnumRefreshTimeLength(quint32 refreshTimelength)
{
    //清空
    data.clear();

    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfe;
    dataStream << (quint8)0x17;

    dataStream << (quint32)refreshTimelength;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;
}

//探测器增益
void CommandHelper::slotDetectorGain(quint8 ch1, quint8 ch2, quint8 ch3, quint8 ch4)
{
    //清空
    data.clear();

    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfb;
    dataStream << (quint8)0x11;

    /*
    十六进制	增益
    00	0.08
    01	0.16
    02	0.32
    03	0.63
    04	1.26
    05	2.52
    06	5.01
    07	10
    */

    dataStream << (quint8)ch4;
    dataStream << (quint8)ch3;
    dataStream << (quint8)ch2;
    dataStream << (quint8)ch1;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;
}

//传输模式
void CommandHelper::slotTransferMode(quint8 ch1, quint8 ch2, quint8 ch3, quint8 ch4, quint8 mode)
{
    //清空
    data.clear();

    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xff;
    dataStream << (quint8)0x10;

    //是否开启相应通道,0关闭，1开启    CH4在高位，    CH3在低位
    dataStream << (quint8)((ch4 & 0x10) | (ch3 & 0x01));
    //是否开启相应通道,0关闭，1开启    CH2在高位，    CH1在低位
    dataStream << (quint8)((ch2 & 0x10) | (ch1 & 0x01));
    dataStream << (quint8)0x00;
    //00:停止 01:软件触发 02:硬件触发
    dataStream << (quint8)mode;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;
}
