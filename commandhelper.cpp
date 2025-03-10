#include "commandhelper.h"
#include <QDataStream>

CommandHelper::CommandHelper(QObject *parent) : QObject(parent)
{
    initSocket(&socketRelay);
    initSocket(&socketDetector);
    initSocket(&socketDisplacement1);
    initSocket(&socketDisplacement2);

    // 创建数据解析线程
    qDebug() << "main thread ID:" << QThread::currentThreadId();
    QUiThread* workhThread = new QUiThread();
    workhThread->setWorkThreadProc([=](){
        slotAnalyzeFrame();
    });
    workhThread->start();
    connect(this, &CommandHelper::destroyed, [=]() {
        workhThread->exit(0);
        workhThread->wait(500);
    });
}

void CommandHelper::initSocket(QTcpSocket** _socket)
{
    QTcpSocket *socket = new QTcpSocket(/*this*/);
    int bufferSize = 4 * 1024 * 1024;
    socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, bufferSize);
    socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, bufferSize);
    *_socket = socket;
}

void CommandHelper::openPower(QString ip, qint32 port)
{
    //断开网络连接
    if (socketRelay->isOpen())
        socketRelay->disconnectFromHost();

    disconnect(socketRelay, nullptr, this, nullptr);// 断开所有槽函数

    //网络异常
    connect(socketRelay, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [=](QAbstractSocket::SocketError){
        emit sigPowerStatus(false);
    });

    //状态改变
    connect(socketRelay, &QAbstractSocket::stateChanged, this, [=](QAbstractSocket::SocketState){
    });

    //连接成功
    connect(socketRelay, &QTcpSocket::connected, this, [=](){
        emit sigPowerStatus(true);

        //发送指令
        socketRelay->write("read");
    });

    //接收数据
    connect(socketRelay, &QTcpSocket::readyRead, this, [=](){
        QByteArray binaryData = socketRelay->readAll();

        QString ack = QString::fromStdString(binaryData.toStdString());
        if (ack.startsWith("relay")){
            QString status = ack.remove("relay");
            bool b1 = status[0] == '1';
            bool b2 = status[1] == '1';
            emit sigRelayStatus(b1, b2);
        }
    });

    socketRelay->connectToHost(ip, port);
}

void CommandHelper::closePower(QString ip, qint32 port)
{
    //断开网络连接
    if (socketRelay->isOpen())
        socketRelay->disconnectFromHost();

    disconnect(socketRelay);// 断开所有槽函数
    emit sigPowerStatus(false);
    return;

    //网络异常
    connect(socketRelay, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [=](QAbstractSocket::SocketError){
        emit sigPowerStatus(false);
    });

    //状态改变
    connect(socketRelay, &QAbstractSocket::stateChanged, this, [=](QAbstractSocket::SocketState){
    });

    //连接成功
    connect(socketRelay, &QTcpSocket::connected, this, [=](){
        emit sigPowerStatus(true);

        //发送指令
        socketRelay->write("close");
    });

    //接收数据
    connect(socketRelay, &QTcpSocket::readyRead, this, [=](){
        QByteArray binaryData = socketRelay->readAll();

        QString ack = QString::fromStdString(binaryData.toStdString());
        if (ack.startsWith("relay")){
            QString status = ack.remove("relay");
            bool b1 = status[0] == '1';
            bool b2 = status[1] == '1';
            emit sigRelayStatus(b1, b2);
        }
    });

    socketRelay->connectToHost(ip, port);
}

void CommandHelper::makeFrame()
{
    //清空
    frame.clear();
    QByteArray data;

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
    command.clear();

    QDataStream dataStream(&command, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfe;
    dataStream << (quint8)0x11;

    dataStream << (quint16)ch2;
    dataStream << (quint16)ch1;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;

    socketDetector->write(command);
    //socketDetector->waitForBytesWritten();
    //socketDetector->flush();
}

//触发阈值2
void CommandHelper::slotTriggerThold2(quint16 ch3, quint16 ch4)
{
    //清空
    command.clear();

    QDataStream dataStream(&command, QIODevice::WriteOnly);
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

    socketDetector->write(command);
}

//波形极性
void CommandHelper::slotWaveformPolarity(quint8 v)
{
    //清空
    command.clear();

    QDataStream dataStream(&command, QIODevice::WriteOnly);
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

    socketDetector->write(command);
}


//波形触发模式
void CommandHelper::slotWaveformTriggerModel(quint8 mode)
{
    //清空
    command.clear();

    QDataStream dataStream(&command, QIODevice::WriteOnly);
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

    socketDetector->write(command);
}

//波形长度
void CommandHelper::slotWaveformLength(quint8 v)
{
    //清空
    command.clear();

    QDataStream dataStream(&command, QIODevice::WriteOnly);
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

    socketDetector->write(command);
}

//能谱模式/粒子模式死时间
void CommandHelper::slotDieTimeLength(quint16 dieTimelength)
{
    //清空
    command.clear();

    QDataStream dataStream(&command, QIODevice::WriteOnly);
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

    socketDetector->write(command);
}

//能谱刷新时间
void CommandHelper::slotSpectnumRefreshTimeLength(quint32 refreshTimelength)
{
    //清空
    command.clear();

    QDataStream dataStream(&command, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfe;
    dataStream << (quint8)0x17;

    dataStream << (quint32)refreshTimelength;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;

    socketDetector->write(command);
}

//探测器增益
void CommandHelper::slotDetectorGain(quint8 ch1, quint8 ch2, quint8 ch3, quint8 ch4)
{
    //清空
    command.clear();

    QDataStream dataStream(&command, QIODevice::WriteOnly);
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

    socketDetector->write(command);
}

//传输模式
void CommandHelper::slotTransferModel(quint8 mode)
{
    //清空
    command.clear();

    QDataStream dataStream(&command, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfa;
    dataStream << (quint8)0x13;

    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    //00:能谱 01:波形 02:粒子
    dataStream << (quint8)mode;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;

    socketDetector->write(command);
}

//启动测量
void CommandHelper::slotStart()
{
    //清空
    command.clear();

    QDataStream dataStream(&command, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xff;
    dataStream << (quint8)0x10;

    quint8 ch1 = 0x01;
    quint8 ch2 = 0x01;
    quint8 ch3 = 0x00;
    quint8 ch4 = 0x00;

    //是否开启相应通道,0关闭，1开启    CH4在高位，    CH3在低位
    dataStream << (quint8)((ch4 & 0x10) | (ch3 & 0x01));
    //是否开启相应通道,0关闭，1开启    CH2在高位，    CH1在低位
    dataStream << (quint8)((ch2 & 0x10) | (ch1 & 0x01));
    dataStream << (quint8)0x00;

    //00:停止 01:软件触发 02:硬件触发
    dataStream << (quint8)0x01;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;

    socketDetector->write(command);
}

//开始手工测量
#include <QThread>
void CommandHelper::slotStartManualMeasure(QString ip, qint32 port, DetectorParameter p)
{
    workStatus = Measuring;//Preparing;
    detectorParameter = p;

    //断开网络连接
    if (socketDetector->isOpen())
        socketDetector->disconnectFromHost();

    // 断开所有槽函数
    disconnect(socketDetector, nullptr, this, nullptr);

    //网络异常
    connect(socketDetector, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [=](QAbstractSocket::SocketError){
        emit sigDetectorStatus(false);
    });

    //状态改变
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    connect(socketDetector, &QAbstractSocket::stateChanged, this, [=](QAbstractSocket::SocketState){
    });

    //连接成功
    connect(socketDetector, &QTcpSocket::connected, this, [=](){
        emit sigDetectorStatus(true);

        if (0 == prepareStep){
            // 触发阈值
            emit slotTriggerThold1(detectorParameter.triggerThold1, detectorParameter.triggerThold2);
        }
    });

    //接收数据
    connect(socketDetector, &QTcpSocket::readyRead, this, [=](){
        QByteArray binaryData = socketDetector->readAll();

        if (workStatus == Preparing){
            if (binaryData.compare(command) == 0){
                prepareStep++;
            }

            switch (prepareStep) {
            case 1: // 波形极性
                emit slotWaveformPolarity(detectorParameter.waveformPolarity);
                break;
            case 2: // 能谱模式/粒子模式死时间
                emit slotDieTimeLength(detectorParameter.dieTimeLength);
                break;
            case 3: // 探测器增益
                emit slotDetectorGain(detectorParameter.gain, detectorParameter.gain, 0x00, 0x00);
                break;
            case 4: // 传输模式
                emit slotTransferModel(detectorParameter.transferModel);
                break;
            case 5: // 开始测量/停止测量
                emit slotStart();
                workStatus = Measuring;
                break;
            }
        } else if (workStatus == Measuring) {
            QMutexLocker locker(&mutex);
            cachePool.push_back(binaryData);
        }
    });

    //连接之前清空缓冲区
    QMutexLocker locker(&mutex);
    cachePool.clear();

    //连接探测器
    prepareStep = 0;
    socketDetector->connectToHost(ip, port);
}

void CommandHelper::slotStopManualMeasure()
{
    if (nullptr == socketDetector)
        return;

    //清空
    command.clear();

    QDataStream dataStream(&command, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xff;
    dataStream << (quint8)0x10;

    quint8 ch1 = 0x00;
    quint8 ch2 = 0x00;
    quint8 ch3 = 0x00;
    quint8 ch4 = 0x00;

    //是否开启相应通道,0关闭，1开启    CH4在高位，    CH3在低位
    dataStream << (quint8)((ch4 & 0x10) | (ch3 & 0x01));
    //是否开启相应通道,0关闭，1开启    CH2在高位，    CH1在低位
    dataStream << (quint8)((ch2 & 0x10) | (ch1 & 0x01));
    dataStream << (quint8)0x00;

    //00:停止 01:软件触发 02:硬件触发
    dataStream << (quint8)0x00;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;

    socketDetector->write(command);
}

#include <QThread>
void CommandHelper::slotDoTasks()
{
    while (!taskFinished)
    {
        QThread::msleep(30);
    }
}

void CommandHelper::slotAnalyzeFrame()
{
    qDebug()<<"work thread ID:"<<QThread::currentThreadId()<<'\n';
    while (!taskFinished)
    {
        QMutexLocker locker(&mutex);
        QByteArray validFrame;

        //00:能谱 03:波形 05:粒子
        if (detectorParameter.transferModel == 0x00){
            if (1){
                // 直接写文件                
            } else {
                qint32 minPkgSize = 8192 * 4;
                bool isNual = false;
                while (true){
                    if (cachePool.size() >= minPkgSize){
                        // 寻找包头
                        if ((quint8)cachePool.at(0) == 0x00 && (quint8)cachePool.at(0) == 0x00 && (quint8)cachePool.at(0) == 0xaa && (quint8)cachePool.at(0) == 0xb3){
                            // 寻找包尾(正常情况包尾正确)
                            if ((quint8)cachePool.at(minPkgSize-4) == 0x00 && (quint8)cachePool.at(minPkgSize-3) == 0x00 && (quint8)cachePool.at(minPkgSize-2) == 0xcc && (quint8)cachePool.at(minPkgSize-1) == 0xd3){
                                isNual = true;
                            }
                        } else {
                            cachePool.remove(0, 1);
                        }
                    }
                }


                if (isNual){
                    //复制有效数据
                    validFrame.append(cachePool.data(), minPkgSize);
                    cachePool.remove(0, minPkgSize);
                    locker.unlock();

                    //处理数据
                    const unsigned char *ptrOffset = (const unsigned char *)validFrame.constData();
                    //能谱序号
                    SequenceNumbre = static_cast<quint32>(ptrOffset[0]) << 24 |
                                      static_cast<quint32>(ptrOffset[1]) << 16 |
                                      static_cast<quint32>(ptrOffset[2]) << 8 |
                                      static_cast<quint32>(ptrOffset[3]);

                    //测量时间(单位：×10ns)
                    ptrOffset += 4;
                    quint32 time = static_cast<quint32>(ptrOffset[0]) << 24 |
                                     static_cast<quint32>(ptrOffset[1]) << 16 |
                                     static_cast<quint32>(ptrOffset[2]) << 8 |
                                     static_cast<quint32>(ptrOffset[3]);

                    //通道号
                    ptrOffset += 4;
                    quint32 channel = static_cast<quint32>(ptrOffset[0]) << 24 |
                                     static_cast<quint32>(ptrOffset[1]) << 16 |
                                     static_cast<quint32>(ptrOffset[2]) << 8 |
                                     static_cast<quint32>(ptrOffset[3]);

                }
            }
        } else if (detectorParameter.transferModel == 0x05){

            qint32 minPkgSize = 1026 * 8;
            bool isNual = false;
            while (true){
                int size = cachePool.size();
                if (size >= minPkgSize){
                    // 寻找包头
                    if ((quint8)cachePool.at(0) == 0x00 && (quint8)cachePool.at(1) == 0x00 && (quint8)cachePool.at(2) == 0xaa && (quint8)cachePool.at(3) == 0xb3){
                        // 寻找包尾(正常情况包尾正确)
                        if ((quint8)cachePool.at(minPkgSize-4) == 0x00 && (quint8)cachePool.at(minPkgSize-3) == 0x00 && (quint8)cachePool.at(minPkgSize-2) == 0xcc && (quint8)cachePool.at(minPkgSize-1) == 0xd3){
                            isNual = true;
                            break;
                        }
                    } else {
                        cachePool.remove(0, 1);
                    }
                } else {
                    break;
                }
            }

            if (isNual){
                //复制有效数据
                validFrame.append(cachePool.data(), minPkgSize);
                cachePool.remove(0, minPkgSize);
                locker.unlock();

                //处理数据
                const unsigned char *ptrOffset = (const unsigned char *)validFrame.constData();

                //通道号(8字节)
                ptrOffset += 4;
                quint64 channel = static_cast<quint64>(ptrOffset[0]) << 56 |
                                 static_cast<quint64>(ptrOffset[1]) << 48 |
                                 static_cast<quint64>(ptrOffset[2]) << 40 |
                                 static_cast<quint64>(ptrOffset[3]) << 32 |
                                 static_cast<quint64>(ptrOffset[4]) << 24 |
                                 static_cast<quint64>(ptrOffset[5]) << 16 |
                                 static_cast<quint64>(ptrOffset[6]) << 8 |
                                 static_cast<quint64>(ptrOffset[7]);

                //粒子模式数据1024*8byte,前6字节:时间,后2字节:能量
                int ref = 1;
                ptrOffset += 8;
                vctFrames.channel = channel;
                while (ref++<=1024){
                    PariticalData v;
                    v.time = static_cast<quint64>(ptrOffset[0]) << 40 |
                            static_cast<quint64>(ptrOffset[1]) << 32 |
                            static_cast<quint64>(ptrOffset[2]) << 24 |
                            static_cast<quint64>(ptrOffset[3]) << 16 |
                            static_cast<quint64>(ptrOffset[4]) << 8 |
                            static_cast<quint64>(ptrOffset[5]);
                    v.data = static_cast<quint16>(ptrOffset[6]) << 8 | static_cast<quint16>(ptrOffset[7]);

                    ptrOffset += 8;
                    vctFrames.data.push_back(v);
                }

                emit sigRefreshData(vctFrames);
            }
        } else if (detectorParameter.transferModel == 0x02){

        } else{
            cachePool.clear();
        }

        QThread::msleep(5);
    }
}
