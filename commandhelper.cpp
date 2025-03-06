#include "commandhelper.h"

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
