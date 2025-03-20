#include "commandhelper.h"
#include "sysutils.h"
#include <QDataStream>
#include <QApplication>
#include <QDateTime>

CommandHelper::CommandHelper(QObject *parent) : QObject(parent)
  , coincidenceAnalyzer(new CoincidenceAnalyzer)
{
    initSocket(&socketDisplacement1);
    initSocket(&socketDisplacement2);

    initSocket(&socketRelay);
    //网络异常
    connect(socketRelay, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [=](QAbstractSocket::SocketError){
        emit sigRelayFault();
    });

    //状态改变
    connect(socketRelay, &QAbstractSocket::stateChanged, this, [=](QAbstractSocket::SocketState){
    });

    //连接成功
    connect(socketRelay, &QTcpSocket::connected, this, [=](){
        //发送查询指令
        command.clear();
        QDataStream dataStream(&command, QIODevice::WriteOnly);
        dataStream << (quint8)0x48;
        dataStream << (quint8)0x3a;
        dataStream << (quint8)0x01;
        dataStream << (quint8)0x53;
        dataStream << (quint8)0x00;
        dataStream << (quint8)0x00;
        dataStream << (quint8)0x00;
        dataStream << (quint8)0x00;
        dataStream << (quint8)0x00;
        dataStream << (quint8)0x00;
        dataStream << (quint8)0x00;
        dataStream << (quint8)0x00;
        dataStream << (quint8)0xd6;
        dataStream << (quint8)0x45;
        dataStream << (quint8)0x44;
        socketRelay->write(command);
    });

    initSocket(&socketDetector);
    socketDetector->setSocketOption(QAbstractSocket::LowDelayOption, 1);//优化Socket以实现低延迟
}

void CommandHelper::startWork()
{
    // 创建数据解析线程
    analyzeNetDataThread = new QUiThread(this);
    analyzeNetDataThread->setObjectName("analyzeNetDataThread");
    analyzeNetDataThread->setWorkThreadProc([=](){
        slotAnalyzeNetFrame();
    });
    analyzeNetDataThread->start();
    connect(this, &CommandHelper::destroyed, [=]() {
        analyzeNetDataThread->exit(0);
        analyzeNetDataThread->wait(500);
    });

    // 图形数据刷新
    plotUpdateThread = new QUiThread(this);
    plotUpdateThread->setObjectName("plotUpdateThread");
    plotUpdateThread->setWorkThreadProc([=](){
        slotPlotUpdateFrame();
    });
    plotUpdateThread->start();
    connect(this, &CommandHelper::destroyed, [=]() {
        plotUpdateThread->exit(0);
        plotUpdateThread->wait(500);
    });
}

CommandHelper::~CommandHelper(){
    taskFinished = true;

    auto closeSocket = [&](QTcpSocket* socket){
        if (socket){
            socket->disconnectFromHost();
            socket->close();
            delete socket;
            socket = nullptr;
        }
    };

    closeSocket(socketDisplacement1);
    closeSocket(socketDisplacement2);
    closeSocket(socketRelay);
    closeSocket(socketDetector);

    delete coincidenceAnalyzer;
    coincidenceAnalyzer = nullptr;
}

void CommandHelper::initSocket(QTcpSocket** _socket)
{
    QTcpSocket *socket = new QTcpSocket(/*this*/);
    int bufferSize = 4 * 1024 * 1024;
    socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, bufferSize);
    socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, bufferSize);
    *_socket = socket;
}

void CommandHelper::openDisplacement(QString ip, qint32 port, qint32 index)
{
    sigDisplacementStatus(true, index);
    return;
    QTcpSocket *socketDisplacement;
    if (0 == index)
        socketDisplacement = socketDisplacement1;
    else
        socketDisplacement = socketDisplacement2;

    //断开网络连接
    if (socketDisplacement->isOpen())
        socketDisplacement->disconnectFromHost();

    disconnect(socketDisplacement);// 断开所有槽函数

    //网络异常
    connect(socketDisplacement, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [=](QAbstractSocket::SocketError){
        emit sigDisplacementFault(index);
    });

    //状态改变
    connect(socketDisplacement, &QAbstractSocket::stateChanged, this, [=](QAbstractSocket::SocketState){
    });

    //连接成功
    connect(socketDisplacement, &QTcpSocket::connected, this, [=](){
        sigDisplacementStatus(true, index);
        //发送位移指令
    });

    //接收数据
    connect(socketDisplacement, &QTcpSocket::readyRead, this, [=](){
    });

    socketDisplacement->connectToHost(ip, port);
}

void CommandHelper::closeDisplacement(qint32 index)
{
    QTcpSocket *socketDisplacement;
    if (0 == index)
        socketDisplacement = socketDisplacement1;
    else
        socketDisplacement = socketDisplacement2;

    socketDisplacement->close();
    sigDisplacementStatus(false, index);
    sigDisplacementStatus(false, index);
}

void CommandHelper::openRelay(QString ip, qint32 port)
{
    //断开网络连接
    if (socketRelay->isOpen())
        socketRelay->disconnectFromHost();

    disconnect(socketRelay, &QTcpSocket::readyRead, this, nullptr);

    socketRelay->setProperty("firstQuery", true);
    //接收数据
    connect(socketRelay, &QTcpSocket::readyRead, this, [&](){
        QByteArray binaryData = socketRelay->readAll();

        // 判断指令只开关返回指令还是查询返回指令
        if (binaryData.size() == 15){
            if (binaryData.at(0) == 0x48 && binaryData.at(1) == 0x3a && binaryData.at(2) == 0x01 && binaryData.at(3) == 0x54){
                if (binaryData.at(4) == 0x00 || binaryData.at(5) == 0x00){
                    //继电器未开启

                    if (!socketRelay->property("firstQuery").toBool()){
                        // 如果已经发送过开启指令，则直接上报状态即可
                        emit sigRelayStatus(false);
                    } else {
                        socketRelay->setProperty("firstQuery", false);

                        //发送开启指令
                        command.clear();
                        QDataStream dataStream(&command, QIODevice::WriteOnly);
                        dataStream << (quint8)0x48;
                        dataStream << (quint8)0x3a;
                        dataStream << (quint8)0x01;
                        dataStream << (quint8)0x57;
                        dataStream << (quint8)0x01;
                        dataStream << (quint8)0x01;
                        dataStream << (quint8)0x00;
                        dataStream << (quint8)0x00;
                        dataStream << (quint8)0x00;
                        dataStream << (quint8)0x00;
                        dataStream << (quint8)0x00;
                        dataStream << (quint8)0x00;
                        dataStream << (quint8)0xdc;
                        dataStream << (quint8)0x45;
                        dataStream << (quint8)0x44;
                        socketRelay->write(command);
                    }
                } else if (binaryData.at(4) == 0x01 || binaryData.at(5) == 0x01){
                    //已经开启
                    emit sigRelayStatus(true);
                }
            }
        }
    });

    socketRelay->connectToHost(ip, port);
}

void CommandHelper::closeRelay()
{
    disconnect(socketRelay, &QTcpSocket::readyRead, this, nullptr);

    //清空
    command.clear();
    QDataStream dataStream(&command, QIODevice::WriteOnly);
    dataStream << (quint8)0x48;
    dataStream << (quint8)0x3a;
    dataStream << (quint8)0x01;
    dataStream << (quint8)0x57;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0xda;
    dataStream << (quint8)0x45;
    dataStream << (quint8)0x44;

    //接收数据
    connect(socketRelay, &QTcpSocket::readyRead, this, [=](){
        QByteArray binaryData = socketRelay->readAll();

        // 判断指令只开关返回指令还是查询返回指令
        if (binaryData.size() == 15){
            if (binaryData.at(0) == 0x48 && binaryData.at(1) == 0x3a && binaryData.at(2) == 0x01 && binaryData.at(3) == 0x54){
                if (binaryData.at(4) == 0x00 || binaryData.at(5) == 0x00){
                    emit sigRelayStatus(false);
                } else if (binaryData.at(4) == 0x01 || binaryData.at(5) == 0x01){
                    //已经开启
                    emit sigRelayStatus(true);

                    socketRelay->disconnectFromHost();
                }
            }
        }
    });

    socketRelay->write(command);
}

void CommandHelper::openDetector(QString ip, qint32 port)
{
    //断开网络连接
    if (socketDetector->isOpen())
        socketDetector->disconnectFromHost();

    disconnect(socketDetector, &QTcpSocket::readyRead, this, nullptr);

    //网络异常
    connect(socketDetector, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [=](QAbstractSocket::SocketError){
        emit sigDetectorFault();
    });

    //状态改变
    connect(socketDetector, &QAbstractSocket::stateChanged, this, [=](QAbstractSocket::SocketState){
    });

    //连接成功
    connect(socketDetector, &QTcpSocket::connected, this, [=](){
        //发送位移指令
        emit sigDetectorStatus(true);
    });

    //接收数据
    connect(socketDetector, &QTcpSocket::readyRead, this, [=](){
    });

    socketDetector->connectToHost(ip, port);
}

void CommandHelper::closeDetector()
{
    //停止测量
    slotStopManualMeasure();

    socketDetector->close();
    emit sigDetectorStatus(false);
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
void CommandHelper::slotStart(qint8 mode)
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
    quint8 ch3 = 0x01;
    quint8 ch4 = 0x01;

    //是否开启相应通道,0关闭，1开启    CH4在高位，    CH3在低位
    dataStream << (quint8)((ch4 << 4) | (ch3 & 0x01));
    //是否开启相应通道,0关闭，1开启    CH2在高位，    CH1在低位
    dataStream << (quint8)((ch2 << 4) | (ch1 & 0x01));
    dataStream << (quint8)0x00;

    //00:停止 01:软件触发 02:硬件触发
    dataStream << (quint8)mode;//0x01;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;

    qDebug() << QString("开始测量，软触发模式");
    socketDetector->write(command);
}

//开始手工测量
#include <QThread>
void CommandHelper::slotStartManualMeasure(DetectorParameter p)
{
    coincidenceAnalyzer->initialize();
    workStatus = Preparing;
    detectorParameter = p;

    // 断开所有槽函数
    disconnect(socketDetector, SIGNAL(readyRead()), this, nullptr);

    //接收数据
    connect(socketDetector, &QTcpSocket::readyRead, this, [=](){
        QByteArray binaryData = socketDetector->readAll();

        //00:能谱 03:波形 05:粒子
        if (workStatus == Preparing){
            if (detectorParameter.transferModel == 0x00 || detectorParameter.transferModel == 0x05){
                if (binaryData.compare(command) == 0){
                    prepareStep++;
                } else if (prepareStep == 5){
                    QByteArray firstPart = binaryData.left(command.size());
                    if (firstPart == command){
                        // 比较成功
                        prepareStep++;
                    }
                }

                switch (prepareStep) {
                case 1: // 波形极性
                    qInfo() << QString("设置波形极性, 值=%1").arg(detectorParameter.waveformPolarity);
                    slotWaveformPolarity(detectorParameter.waveformPolarity);
                    break;
                case 2: // 能谱模式/粒子模式死时间
                    if (detectorParameter.transferModel == 0x00){
                        qInfo() << QString("设置能谱刷新时间，值=%1").arg(detectorParameter.refreshTimeLength);
                        slotSpectnumRefreshTimeLength(detectorParameter.refreshTimeLength);
                    } else if (detectorParameter.transferModel == 0x03 || detectorParameter.transferModel == 0x05){
                        qInfo() << QString("设置能谱模式/粒子模式死时间，值=%1").arg(detectorParameter.dieTimeLength);
                        slotDieTimeLength(detectorParameter.dieTimeLength);
                    }
                    break;
                case 3: // 探测器增益
                    qInfo() << QString("设置增益，值=%1").arg(detectorParameter.dieTimeLength);
                    slotDetectorGain(detectorParameter.gain, detectorParameter.gain, 0x00, 0x00);
                    break;
                case 4: // 传输模式
                    qInfo() << QString("设置传输模式，值=%1").arg(detectorParameter.transferModel);
                    slotTransferModel(detectorParameter.transferModel);
                    break;
                case 5: // 开始测量/停止测量
                    slotStart(0x01);
                    break;
                case 6: // 开始测量/停止测量
                    sigMeasureStart(detectorParameter.measureModel);
                    workStatus = Measuring;
                    binaryData.remove(0, command.size());
                    break;
                }
            } else if (detectorParameter.transferModel == 0x03) {
                if (binaryData.compare(command) == 0){
                    prepareStep++;
                } else if (prepareStep == 7){
                    QByteArray firstPart = binaryData.left(command.size());
                    if (firstPart == command){
                        // 比较成功
                        prepareStep++;
                    }
                }

                switch (prepareStep) {
                case 1: // 波形极性
                    qInfo() << QString("设置波形极性, 值=%1").arg(detectorParameter.waveformPolarity);
                    slotWaveformPolarity(detectorParameter.waveformPolarity);
                    break;
                case 2: // 能谱模式/粒子模式死时间
                    qInfo() << QString("设置能谱模式/粒子模式死时间，值=%1").arg(detectorParameter.dieTimeLength);
                    slotDieTimeLength(detectorParameter.dieTimeLength);
                    break;
                case 3: // 探测器增益
                    qInfo() << QString("设置增益，值=%1").arg(detectorParameter.dieTimeLength);
                    slotDetectorGain(detectorParameter.gain, detectorParameter.gain, 0x00, 0x00);
                    break;
                case 4: // 波形长度
                    qInfo() << QString("设置波形长度，值=%1").arg(detectorParameter.waveLength);
                    slotWaveformLength(detectorParameter.waveLength);
                    break;
                case 5: // 波形触发模式
                    qInfo() << QString("设置波形触发模式，值=%1").arg(detectorParameter.waveLength);
                    slotWaveformTriggerModel(detectorParameter.triggerModel);
                    break;
                case 6: // 传输模式
                    qInfo() << QString("设置传输模式，值=%1").arg(detectorParameter.transferModel);
                    slotTransferModel(detectorParameter.transferModel);
                    break;
                case 7: // 开始测量/停止测量
                    slotStart(0x01);
                    break;
                case 8: // 开始测量/停止测量
                    emit sigMeasureStart(detectorParameter.measureModel);
                    workStatus = Measuring;
                    binaryData.remove(0, command.size());
                    break;
                }
            }
        }

        if (workStatus == Measuring) {
            if (nullptr != pfSave && binaryData.size() > 0){
                pfSave->write(binaryData);
                pfSave->flush();
                emit sigRecvData(binaryData.size());
            }

            // 只有符合模式才需要做进一步数据处理
            if (detectorParameter.transferModel == 0x05){
                QMutexLocker locker(&mutexCache);
                cachePool.push_back(binaryData);
            }
        }
    });

    //连接之前清空缓冲区
    QMutexLocker locker(&mutexCache);
    cachePool.clear();
    handlerPool.clear();

    QMutexLocker locker2(&mutexReset);
    currentClockStepNs[0] = 1;//fpga时钟步长（数据包的时长）
    currentClockStepNs[1] = 1;

    currentRefreshStepNs[0] = 1;//ui时钟步长（界面刷新时长）
    currentRefreshStepNs[1] = 1;

    currentStepSpectrumFrame[0].dataT.clear(); // 保存当前时长内的时间、能谱信息，一旦达到1s时长，将交给接口处理
    currentStepSpectrumFrame[0].dataE.clear();
    currentStepSpectrumFrame[1].dataT.clear();
    currentStepSpectrumFrame[1].dataE.clear();

    currentSpectrumFrames.clear();
    currentStepCountFrames.clear();
    totalStepCountFrames.clear();
    totalSpectrumFrames.clear();

    //连接探测器
    prepareStep = 0;
    if (0 == prepareStep){
        currentFilename = QString("%1").arg(defaultCacheDir + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd HHmmss") + ".dat");
        if (nullptr != pfSave){
            pfSave->close();
            delete pfSave;
            pfSave = nullptr;
        }

        pfSave = new QFile(currentFilename);
        if (pfSave->open(QIODevice::WriteOnly)) {
            qInfo() << tr("创建缓存文件成功，文件名：%1").arg(currentFilename);
        } else {
            qWarning() << tr("创建缓存文件失败，文件名：%1").arg(currentFilename);
        }

        // 触发阈值
        qDebug() << QString("设置触发阈值, 值1=%1 值2=%2").arg(detectorParameter.triggerThold1).arg(detectorParameter.triggerThold2);
        slotTriggerThold1(detectorParameter.triggerThold1, detectorParameter.triggerThold2);
    }
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

    qint64 writeLen = socketDetector->write(command);
    while (!writeLen){
        writeLen = socketDetector->write(command);
    }
}

//开始能谱测量
void CommandHelper::slotStartAutoMeasure(DetectorParameter p)
{
    coincidenceAnalyzer->initialize();
    workStatus = Preparing;
    detectorParameter = p;

    // 断开所有槽函数
    disconnect(socketDetector, SIGNAL(readyRead()), this, nullptr);

    //接收数据
    connect(socketDetector, &QTcpSocket::readyRead, this, [=](){
        QByteArray binaryData = socketDetector->readAll();

        //00:能谱 03:波形 05:粒子
        if (workStatus == Preparing){
            if (detectorParameter.transferModel == 0x00 || detectorParameter.transferModel == 0x05){
                if (binaryData.compare(command) == 0){
                    prepareStep++;
                } else if (prepareStep == 5){
                    //清空
                    command.clear();

                    QDataStream dataStream(&command, QIODevice::WriteOnly);
                    dataStream << (quint8)0x12;
                    dataStream << (quint8)0x34;
                    dataStream << (quint8)0x00;
                    dataStream << (quint8)0xaa;
                    dataStream << (quint8)0x00;
                    dataStream << (quint8)0x0c;

                    dataStream << (quint8)0x00;
                    dataStream << (quint8)0x00;
                    dataStream << (quint8)0x00;
                    dataStream << (quint8)0x00;

                    dataStream << (quint8)0xab;
                    dataStream << (quint8)0xcd;

                    QByteArray firstPart = binaryData.left(command.size());
                    if (firstPart == command){
                        // 比较成功
                        prepareStep++;
                    }
                }

                switch (prepareStep) {
                case 1: // 波形极性
                    qDebug() << QString("设置波形极性, 值=%1").arg(detectorParameter.waveformPolarity);
                    slotWaveformPolarity(detectorParameter.waveformPolarity);
                    break;
                case 2: // 能谱模式/粒子模式死时间
                    if (detectorParameter.transferModel == 0x00){
                        qDebug() << QString("设置能谱刷新时间，值=%1").arg(detectorParameter.refreshTimeLength);
                        slotSpectnumRefreshTimeLength(detectorParameter.refreshTimeLength);
                    } else if (detectorParameter.transferModel == 0x03 || detectorParameter.transferModel == 0x05){
                        qDebug() << QString("设置能谱模式/粒子模式死时间，值=%1").arg(detectorParameter.dieTimeLength);
                        slotDieTimeLength(detectorParameter.dieTimeLength);
                    }
                    break;
                case 3: // 探测器增益
                    qDebug() << QString("设置增益，值=%1").arg(detectorParameter.dieTimeLength);
                    slotDetectorGain(detectorParameter.gain, detectorParameter.gain, 0x00, 0x00);
                    break;
                case 4: // 传输模式
                    qDebug() << QString("设置传输模式，值=%1").arg(detectorParameter.transferModel);
                    slotTransferModel(detectorParameter.transferModel);
                    break;
                case 5: // 开始测量/停止测量
                    slotStart(0x02);
                    break;
                case 6: // 开始测量/停止测量
                    workStatus = Measuring;
                    emit sigMeasureStart(detectorParameter.measureModel);
                    binaryData.remove(0, command.size());
                    break;
                }
            } else if (detectorParameter.transferModel == 0x03) {
                if (binaryData.compare(command) == 0){
                    prepareStep++;
                } else if (prepareStep == 7){
                    QByteArray firstPart = binaryData.left(command.size());
                    if (firstPart == command){
                        // 比较成功
                        prepareStep++;
                    }
                }

                switch (prepareStep) {
                case 1: // 波形极性
                    qDebug() << QString("设置波形极性, 值=%1").arg(detectorParameter.waveformPolarity);
                    slotWaveformPolarity(detectorParameter.waveformPolarity);
                    break;
                case 2: // 能谱模式/粒子模式死时间
                    qDebug() << QString("设置能谱模式/粒子模式死时间，值=%1").arg(detectorParameter.dieTimeLength);
                    slotDieTimeLength(detectorParameter.dieTimeLength);
                    break;
                case 3: // 探测器增益
                    qDebug() << QString("设置增益，值=%1").arg(detectorParameter.dieTimeLength);
                    slotDetectorGain(detectorParameter.gain, detectorParameter.gain, 0x00, 0x00);
                    break;
                case 4: // 波形长度
                    qDebug() << QString("设置波形长度，值=%1").arg(detectorParameter.waveLength);
                    slotWaveformLength(detectorParameter.waveLength);
                    break;
                case 5: // 波形触发模式
                    qDebug() << QString("设置波形触发模式，值=%1").arg(detectorParameter.waveLength);
                    slotWaveformTriggerModel(detectorParameter.triggerModel);
                    break;
                case 6: // 传输模式
                    qDebug() << QString("设置传输模式，值=%1").arg(detectorParameter.transferModel);
                    slotTransferModel(detectorParameter.transferModel);
                    break;
                case 7: // 开始测量/停止测量
                    slotStart(0x02);
                    break;
                case 8: // 开始测量/停止测量
                    emit sigMeasureStart(detectorParameter.measureModel);
                    workStatus = Measuring;
                    binaryData.remove(0, command.size());
                    break;
                }
            }
        }

        if (workStatus == Measuring) {
            if (nullptr != pfSave){
                pfSave->write(binaryData);
            }

            // 只有符合模式才需要做进一步数据处理
//            if (detectorParameter.transferModel == 0x05){
//                QMutexLocker locker(&mutexCache);
//                cachePool.push_back(binaryData);
//            }
        }
    });

    //连接之前清空缓冲区
    QMutexLocker locker(&mutexCache);
    cachePool.clear();
    handlerPool.clear();

    //连接探测器
    prepareStep = 0;
    if (0 == prepareStep){
        currentFilename = QString("%1").arg(defaultCacheDir + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd HHmmss") + ".dat");
        if (nullptr != pfSave){
            pfSave->close();
            delete pfSave;
            pfSave = nullptr;
        }

        pfSave = new QFile(currentFilename);
        if (pfSave->open(QIODevice::WriteOnly)) {
            qDebug() << tr("创建缓存文件成功，文件名：%1").arg(currentFilename);
        } else {
            qWarning() << tr("创建缓存文件失败，文件名：%1").arg(currentFilename);
        }

        // 触发阈值
        qDebug() << QString("设置触发阈值, 值1=%1 值2=%2").arg(detectorParameter.triggerThold1).arg(detectorParameter.triggerThold2);
        slotTriggerThold1(detectorParameter.triggerThold1, detectorParameter.triggerThold2);
    }
}

void CommandHelper::slotStopAutoMeasure()
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

    qint64 writeLen = socketDetector->write(command);
    while (!writeLen){
        writeLen = socketDetector->write(command);
    }
}

#include <QThread>
void CommandHelper::slotDoTasks()
{
    while (!taskFinished)
    {
        QThread::msleep(30);
    }
}

void CommandHelper::slotAnalyzeNetFrame()
{
//    detectorParameter.transferModel = 0x05;
//    leftE[0] = 3000; leftE[1] = 3000;
//    rightE[0] = 4000; rightE[1] = 4000;

//    QFile qFile("C:/Users/Administrator/Desktop/川大项目/缓存目录/2025-03-19 093103.dat");
//    if (qFile.open(QIODevice::ReadOnly)){
//        handlerPool = qFile.readAll();
//        qFile.close();
//    }

    while (!taskFinished)
    {
        {
            QMutexLocker locker(&mutexCache);
            if (cachePool.size() <= 0){
                if (handlerPool.size() < 12){
                    // 数据单位最小值为12（一个指令长度）
                    QThread::msleep(1);
                }
            } else {
                handlerPool.append(cachePool);
                cachePool.clear();
            }
        }

        QByteArray validFrame;

        //00:能谱 03:波形 05:粒子
        if (detectorParameter.transferModel == 0x00){
            if (1){
                // 直接写文件                
            } else {
                qint32 minPkgSize = 8192 * 4;
                bool isNual = false;
                while (true){
                    if (handlerPool.size() >= minPkgSize){
                        // 寻找包头
                        if ((quint8)handlerPool.at(0) == 0x00 && (quint8)handlerPool.at(0) == 0x00 && (quint8)handlerPool.at(0) == 0xaa && (quint8)handlerPool.at(0) == 0xb3){
                            // 寻找包尾(正常情况包尾正确)
                            if ((quint8)handlerPool.at(minPkgSize-4) == 0x00 && (quint8)handlerPool.at(minPkgSize-3) == 0x00 && (quint8)handlerPool.at(minPkgSize-2) == 0xcc && (quint8)handlerPool.at(minPkgSize-1) == 0xd3){
                                isNual = true;
                            }
                        } else {
                            // 前面几个字节是开始测量的返回码
                            handlerPool.remove(0, 1);
                        }
                    }
                }


                if (isNual){
                    //复制有效数据
                    validFrame.append(handlerPool.data(), minPkgSize);
                    handlerPool.remove(0, minPkgSize);

                    //处理数据
                    const unsigned char *ptrOffset = (const unsigned char *)validFrame.constData();
                    //能谱序号
                    SequenceNumber = static_cast<quint32>(ptrOffset[0]) << 24 |
                                      static_cast<quint32>(ptrOffset[1]) << 16 |
                                      static_cast<quint32>(ptrOffset[2]) << 8 |
                                      static_cast<quint32>(ptrOffset[3]);

                    //测量时间(单位：×10ns)
//                    ptrOffset += 4;
//                    quint32 time = static_cast<quint32>(ptrOffset[0]) << 24 |
//                                     static_cast<quint32>(ptrOffset[1]) << 16 |
//                                     static_cast<quint32>(ptrOffset[2]) << 8 |
//                                     static_cast<quint32>(ptrOffset[3]);

//                    //通道号
//                    ptrOffset += 4;
//                    quint32 channel = static_cast<quint32>(ptrOffset[0]) << 24 |
//                                     static_cast<quint32>(ptrOffset[1]) << 16 |
//                                     static_cast<quint32>(ptrOffset[2]) << 8 |
//                                     static_cast<quint32>(ptrOffset[3]);

                }
            }
        } else if (detectorParameter.transferModel == 0x05){
            const quint32 minPkgSize = 1026 * 8;
            bool isNual = false;
            while (true){
                quint32 size = handlerPool.size();
                if (size >= minPkgSize){
                    // 寻找包头
                    if ((quint8)handlerPool.at(0) == 0x00 && (quint8)handlerPool.at(1) == 0x00 && (quint8)handlerPool.at(2) == 0xaa && (quint8)handlerPool.at(3) == 0xb3){
                        // 寻找包尾(正常情况包尾正确)
                        if ((quint8)handlerPool.at(minPkgSize-4) == 0x00 && (quint8)handlerPool.at(minPkgSize-3) == 0x00 && (quint8)handlerPool.at(minPkgSize-2) == 0xcc && (quint8)handlerPool.at(minPkgSize-1) == 0xd3){
                            isNual = true;
                            break;
                        } else {
                            handlerPool.remove(0, 1);
                        }
                    } else {
                        handlerPool.remove(0, 1);
                    }
                } else if (handlerPool.size() == 12){
                    //12 34 00 0F FF 10 00 11 00 00 AB CD
                    //通过指令来判断测量是否停止
                    if ((quint8)handlerPool.at(0) == 0x12 && (quint8)handlerPool.at(1) == 0x34
                            && (quint8)handlerPool.at(10) == 0xab && (quint8)handlerPool.at(11) == 0xcd){
                        handlerPool.remove(0, 12);

                        if (nullptr != pfSave){
                            pfSave->close();
                            delete pfSave;
                            pfSave = nullptr;
                        }

                        emit sigMeasureStop();
                        break;
                    } else {
                        handlerPool.remove(0, 1);
                    }
                } else {
                    break;
                }
            }

            if (isNual){
                //复制有效数据
                validFrame.append(handlerPool.data(), minPkgSize);
                handlerPool.remove(0, minPkgSize);

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

                //通道值转换
                channel = (channel == 0xFFF1) ? 0 : 1;

                //粒子模式数据1024*8byte,前6字节:时间,后2字节:能量
                int ref = 1;
                ptrOffset += 8;

                std::vector<TimeEnergy> temp;
                while (ref++<=1024){
                    long long t = static_cast<quint64>(ptrOffset[0]) << 40 |
                            static_cast<quint64>(ptrOffset[1]) << 32 |
                            static_cast<quint64>(ptrOffset[2]) << 24 |
                            static_cast<quint64>(ptrOffset[3]) << 16 |
                            static_cast<quint64>(ptrOffset[4]) << 8 |
                            static_cast<quint64>(ptrOffset[5]);
                    unsigned int e = static_cast<quint16>(ptrOffset[6]) << 8 | static_cast<quint16>(ptrOffset[7]);
                    ptrOffset += 8;

                    temp.push_back(TimeEnergy(t, e));
                }

                //数据分拣完毕
                {
                    QMutexLocker locker(&mutexPlot);
                    DetTimeEnergy detTimeEnergy;
                    detTimeEnergy.channel = channel;
                    detTimeEnergy.timeEnergy.swap(temp);
                    currentSpectrumFrames.push_back(detTimeEnergy);
                }
            }
        } else if (detectorParameter.transferModel == 0x03){

        } else {
            handlerPool.clear();
        }

        QThread::msleep(5);
    }
}

void CommandHelper::slotPlotUpdateFrame()
{
    QDateTime tmStart = QDateTime::currentDateTime().addDays(-1);
    const quint64 currentStepBaseNs[2] = {(quint64)1e8, (quint64)1e8};
    currentClockStepNs[0] = 1;
    currentClockStepNs[1] = 1;
    currentRefreshStepNs[0] = 1;
    currentRefreshStepNs[1] = 1;

    while (!taskFinished)
    {
        // 时间步长到了，该刷新界面了
        if (0/*this->refModel*/){
            // 对输入的粒子[时间、能量]数据进行统计,给出计数率随时间变化的曲线，注意这里dataT与dataE一定是相同的长度
            // dataT粒子的时间, 单位ns
            // dataE粒子的能量
            // stepT统计的时间步长, 单位s
            // leftE：左区间
            // rightE：右区间
            // return：vector<TimeCountRate> 计数率数组
            QDateTime tmNow = QDateTime::currentDateTime();
            if (tmStart.msecsTo(tmNow) >= 10){
                tmStart = tmNow;

                //1、把分拣之后的数据全部交换出来，让网络继续接收处理
                std::deque<DetTimeEnergy> swapFrames;
                {
                    QMutexLocker locker(&mutexPlot);
                    totalSpectrumFrames.insert(totalSpectrumFrames.end(), currentSpectrumFrames.begin(), currentSpectrumFrames.end());
                    swapFrames.swap(currentSpectrumFrames);
                }                

                QMutexLocker locker(&mutexReset);
                //2、处理缓存区数据
                if (swapFrames.size() > 0){
                    while (!swapFrames.empty()){
                        DetTimeEnergy detTimeEnergy = swapFrames.front();
                        swapFrames.pop_front();
                        quint8 channel = detTimeEnergy.channel;

                        // 根据步长，将数据添加到当前处理缓存
                        while (!detTimeEnergy.timeEnergy.empty()){
                            TimeEnergy timeEnergy = detTimeEnergy.timeEnergy.front();
                            detTimeEnergy.timeEnergy.pop_front();

                            if (timeEnergy.time > currentStepBaseNs[channel] * currentClockStepNs[channel]){
                                //满足1s时间步长
                                //判断中间是否丢包
                                quint32 clockStepNs = timeEnergy.time / currentStepBaseNs[detTimeEnergy.channel];
                                qDebug() << currentClockStepNs[channel] << "" << clockStepNs;
                                if (clockStepNs > currentClockStepNs[channel]){
                                    //丢包了，补齐丢掉的包
                                    for (quint32 i=currentClockStepNs[channel]; i<clockStepNs; ++i){
                                        long long t = currentStepBaseNs[channel] * i;
                                        unsigned short e = 0;

                                        //空包填充
                                        preStepSpectrumFrame[channel].timeEnergy.push_back(StepTimeEnergy(t, e));

//                                        //计数
//                                        DetStepTimeEnergy cacheCountFrame;
//                                        cacheCountFrame.channel = channel;
//                                        cacheCountFrame = i;//currentClockStepNs[frame.channel];
//                                        cacheCountFrame.dataT = i;//currentClockStepNs[frame.channel];
//                                        cacheCountFrame.dataE = 0; //单位cps
//                                        totalStepCountFrames.push_back(cacheCountFrame);

//                                        //能谱
//                                        PariticalSpectrumFrame cacheSpectrumFrame;
//                                        cacheSpectrumFrame.channel = channel;
//                                        cacheSpectrumFrame.stepT = i;//currentClockStepNs[frame.channel];
//                                        //cacheSpectrumFrame.dataT.push_back(currentClockStepNs[frame.channel]);
//                                        //cacheSpectrumFrame.dataE = GetSpectrum(preStepSpectrumFrame[frame.channel].dataE, maxEnergy, maxCh);
//                                        cacheSpectrumFrame.dataT.insert(cacheSpectrumFrame.dataT.begin(), preStepSpectrumFrame[channel].dataT.begin(), preStepSpectrumFrame[channel].dataT.end());
//                                        cacheSpectrumFrame.dataE.insert(cacheSpectrumFrame.dataE.begin(), preStepSpectrumFrame[channel].dataE.begin(), preStepSpectrumFrame[channel].dataE.end());
//                                        totalStepSpectrumFrames.push_back(cacheSpectrumFrame);

                                        //符合

                                        currentClockStepNs[channel]++;
                                    }
                                } else {

                                }
                                /////////////////////////////////////////////////////////////////////////////////////////

                                //计数
                                vector<unsigned short> dataE;
                                for (quint32 i = 0; i<preStepSpectrumFrame[channel].energy.size(); ++i){
                                    dataE.push_back(preStepSpectrumFrame[channel].energy.at(i));
                                }
                                {
                                    StepTimeCount stepTimeCount;
                                    stepTimeCount.time = clockStepNs;
                                    stepTimeCount.count = GetCount(dataE, this->leftE[channel], this->rightE[channel]); //单位cps

                                    //等到处理完成了再扔进总堆栈里面去
                                    //totalStepCountFrames[channel].push_back(stepTimeCount);

                                    currentStepCountFrames.push_back(stepTimeCount);
                                }
                                /////////////////////////////////////////////////////////////////////////////////////////

                                //能谱
                                {
                                    StepTimeEnergy stepTimeEnergy;
                                    stepTimeEnergy.time = clockStepNs;
                                    stepTimeEnergy.energy = GetSpectrum(dataE, maxEnergy, maxCh);

                                    //等到处理完成了再扔进总堆栈里面去
                                    //totalStepSpectrumFrames[channel].push_back(stepTimeEnergy);

                                    currentStepSpectrumFrames.push_back(stepTimeEnergy);
                                }
                                /////////////////////////////////////////////////////////////////////////////////////////

                                //符合结果
                                //注意：这里需要注意，第2个探测器必须等到第1个探测器的的数据到了，才能一起进行计算
//                                vector<TimeEnergy> data1_2, data2_2;
//                                for (int i=0; i<cacheSpectrumFrame.dataT.size(); ++i){

//                                }
//                                data1_2.push_back({900LL,500});
//                                CoinResult result = coincidenceAnalyzer->Coincidence(data1_2, data2_2, leftE[0], rightE[0], timeWidth);
//                                //删除容器中已经处理的数据点
//                                if (result.dataPoint1 <= (int)data1_2.size()) {
//                                    data1_2.erase(data1_2.begin(), data1_2.begin() + result.dataPoint1);
//                                } else {
//                                    data1_2.clear(); // 如果N大于容器的大小，清空容器
//                                }
//                                if (result.dataPoint2 <= (int)data2_2.size()) {
//                                    data2_2.erase(data2_2.begin(), data2_2.begin() + result.dataPoint2);
//                                } else {
//                                    data2_2.clear(); // 如果N大于容器的大小，清空容器
//                                }

                                // 准备计算
//                                int nanoseconds = 1000000000LL; //1s = 1E9ns
//                                int time1_elapseFPGA;//计算FPGA当前最大时间与上一时刻的时间差,单位：秒
//                                int time2_elapseFPGA;//计算FPGA当前最大时间与上一时刻的时间差,单位：秒
//                                time1_elapseFPGA = data1_2.back().time/nanoseconds - coincidenceAnalyzer->GetcountCoin();//计算FPGA当前最大时间与上一时刻的时间差
//                                time2_elapseFPGA = data2_2.back().time/nanoseconds - coincidenceAnalyzer->GetcountCoin();//计算FPGA当前最大时间与上一时刻的时间差
//                                long long lastTime1 = data1_2.back().time;
//                                long long lastTime2 = data2_2.back().time;
//                                time1_elapseFPGA = lastTime1/nanoseconds - coincidenceAnalyzer->GetcountCoin();//计算FPGA当前最大时间与上一时刻的时间差
//                                time2_elapseFPGA = lastTime2/nanoseconds - coincidenceAnalyzer->GetcountCoin();//计算FPGA当前最大时间与上一时刻的时间差
                                /////////////////////////////////////////////////////////////////////////////////////////

                                preStepSpectrumFrame[channel].energy.clear();


                                /////////////////////////////////////////////////////////////////////////////////////////

                                if (timeEnergy.time > currentStepBaseNs[channel] * currentRefreshStepNs[channel] * this->stepT){
                                    //满足ui设定刷新时间步长
                                    //计数
                                    if (this->refModel){
                                        StepTimeCount sFrame;
                                        sFrame.time = currentRefreshStepNs[channel] * this->stepT;
                                        sFrame.count = 0;
                                        for (auto stepTimeCount : currentStepCountFrames){
                                            sFrame.count += stepTimeCount.count; // 求和
                                        }

                                        //求均值
                                        sFrame.count = sFrame.count / this->stepT;

                                        currentStepCountFrames.clear();
                                        emit sigRefreshCountData(channel, sFrame);
                                    } else {
                                        //能谱
                                        StepTimeEnergy sFrame;
                                        sFrame.time = currentRefreshStepNs[channel] * this->stepT;
                                        sFrame.energy.resize(currentStepSpectrumFrames.at(0).energy.size());
                                        for (auto stepTimeCount : currentStepSpectrumFrames){
                                            for (quint32 i=0; i<sFrame.energy.size(); ++i){
                                                sFrame.energy[i] += stepTimeCount.energy[i]; // 求和
                                            }
                                        }
                                        currentStepSpectrumFrames.clear();
                                        emit sigRefreshSpectrum(channel, sFrame);
                                    }

                                    currentRefreshStepNs[channel]++;
                                }

                                //处理剩余数据
                                currentClockStepNs[channel]++;
                                preStepSpectrumFrame[channel].energy.push_back(timeEnergy.energy);
                            } else {
                                preStepSpectrumFrame[channel].time = currentClockStepNs[channel];
                                preStepSpectrumFrame[channel].energy.push_back(timeEnergy.energy);
                            }
                        }
                    }
                }
            }
        } else {
            QDateTime tmNow = QDateTime::currentDateTime();
            if (tmStart.secsTo(tmNow) >= 1){
                tmStart = tmNow;
                std::deque<DetTimeEnergy> swapFrames;
                {
                    QMutexLocker locker(&mutexPlot);
                    swapFrames.swap(currentSpectrumFrames);
                }

                QMutexLocker locker(&mutexReset);
                //2、处理缓存区数据
                if (swapFrames.size() > 0){
                    vector<TimeEnergy> data1_2, data2_2;
                    while (!swapFrames.empty()){
                        DetTimeEnergy detTimeEnergy = swapFrames.front();
                        swapFrames.pop_front();

                        // 根据步长，将数据添加到当前处理缓存
                        quint8 channel = detTimeEnergy.channel;
                        while (!detTimeEnergy.timeEnergy.empty()){
                            TimeEnergy timeEnergy = detTimeEnergy.timeEnergy.front();
                            detTimeEnergy.timeEnergy.pop_front();
                            if (channel == 0x00){
                                data1_2.push_back(data1_2.begin(), detTimeEnergy.timeEnergy.begin(), detTimeEnergy.timeEnergy.end());
                            } else {
                                data2_2.push_back(data1_2.begin(), detTimeEnergy.timeEnergy.begin(), detTimeEnergy.timeEnergy.end());
                            }
                        }
                    }

                    coincidenceAnalyzer->calculate(data1_2, data2_2, leftE[0], rightE[0], leftE[1], rightE[1], timeWidth);
                    // 读取数据
                    vector<CoincidenceResult> r1 = coincidenceAnalyzer->GetCoinResult();
                    if (r1.size() > 0){
                    }

                    queue<SingleSpectrum> r2 = coincidenceAnalyzer->GetSpectrum();
                    if (r2.size() > 0){
                    }

                    vector<CurrentPoint> r3 = coincidenceAnalyzer->GetPointPerSeconds();
                    if (r3.size() > 0){
                    }
                }
            }
        }

        QThread::msleep(5);
    }
}

void CommandHelper::updateParamter(int _stepT, int _leftE[2], int _rightE[2], bool reset/* = false*/)
{
    QMutexLocker locker(&mutexReset);
    this->stepT = _stepT;
    this->leftE[0] = _leftE[0];
    this->leftE[1] = _leftE[1];
    this->rightE[0] = _rightE[0];
    this->rightE[1] = _rightE[1];

    currentRefreshStepNs[0] = 1;//ui时钟步长（界面刷新时长）
    currentRefreshStepNs[1] = 1;

//    currentStepSpectrumFrame[0].dataT.clear(); // 保存当前时长内的时间、能谱信息，一旦达到1s时长，将交给接口处理
//    currentStepSpectrumFrame[0].dataE.clear();
//    currentStepSpectrumFrame[1].dataT.clear();
//    currentStepSpectrumFrame[1].dataE.clear();

//    currentStepCountFrames.clear();
//    currentStepSpectrumFrames.clear();

//    if (reset){
//        totalStepCountFrames.clear();
//    }

//    //计数模式需要重新刷新图像
//    if (this->refModel){
//        for (auto frame : totalStepCountFrames){
//            if (frame.dataT > currentRefreshStepNs[frame.channel] * this->stepT){
//                PariticalCountFrame sFrame;
//                sFrame.channel = frame.channel;
//                sFrame.dataT = currentRefreshStepNs[frame.channel] * this->stepT;
//                sFrame.dataE = 0;
//                for (auto a : currentStepCountFrames){
//                    sFrame.dataE += a.dataE;
//                }

//                currentRefreshStepNs[frame.channel]++;
//                currentStepCountFrames.clear();
//                emit sigRefreshCountData(sFrame);

//                //未处理的数据一定要放回缓存
//                currentStepCountFrames.push_back(frame);
//            } else {
//                currentStepCountFrames.push_back(frame);
//            }
//        }
//    } else {
//        for (auto frame : totalStepSpectrumFrames){
//            if (frame.stepT > currentRefreshStepNs[frame.channel] * this->stepT){
//                PariticalSpectrumFrame sFrame;
//                sFrame.channel = frame.channel;
//                sFrame.dataE = GetSpectrum(currentStepSpectrumFrames[frame.channel].dataE, maxEnergy, maxCh);

//                currentRefreshStepNs[frame.channel]++;
//                currentStepSpectrumFrames.clear();
//                emit sigRefreshSpectrum(sFrame);

//                //未处理的数据一定要放回缓存
//                currentStepSpectrumFrames.push_back(frame);
//            } else {
//                currentStepSpectrumFrames.push_back(frame);
//            }
//        }
//    }
}

void CommandHelper::switchShowModel(bool _refModel)
{
    this->refModel = _refModel;
}

#include <QMessageBox>
void CommandHelper::saveFileName(QString dstPath)
{
    if (dstPath.endsWith(".dat")){
        QFile file(dstPath);
        if (file.exists()) {
            file.remove();
        }

        //进行文件copy
        if (QFile::copy(currentFilename, dstPath))        
            QMessageBox::information(nullptr, tr("提示"), tr("数据保存成功！"), QMessageBox::Ok, QMessageBox::Ok);
        else
            QMessageBox::information(nullptr, tr("提示"), tr("数据保存失败！"), QMessageBox::Ok, QMessageBox::Ok);
        //QFile::remove(currentFilename);
    } else if (dstPath.endsWith(".txt")) {
        // 将源文件转换为文本文件
        QFile fileSrc(currentFilename);
        if (fileSrc.open(QIODevice::ReadOnly)) {
            QFile fileDst(dstPath);
            if (fileDst.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QDataStream aStream(&fileDst);

                qint32 cachelen = 0;
                qint32 framelen = 1026*8;
                qint32 offset = 0;
                unsigned char *buf = new unsigned char[framelen*2];
                qint32 readlen = fileSrc.read((char*)buf, framelen);
                while (readlen > 0){
                    cachelen += readlen;

                    while (cachelen > framelen){
                        unsigned char* ptrOffset = (unsigned char*)buf;
                        unsigned char* ptrHead = (unsigned char*)buf;
                        unsigned char* ptrTail = (unsigned char*)buf + cachelen;
                        if (ptrOffset[framelen-4] == 0x00 && ptrOffset[framelen-3] == 0x00 && ptrOffset[framelen-2] == 0xaa && ptrOffset[framelen-1] == 0xb3){
                            if (ptrOffset[framelen-4] == 0x00 && ptrOffset[framelen-3] == 0x00 && ptrOffset[framelen-2] == 0xcc && ptrOffset[framelen-1] == 0xd3){
                                //完整帧
                                ptrOffset += 4;
                                quint64 channel = static_cast<quint64>(ptrOffset[0]) << 56 |
                                                 static_cast<quint64>(ptrOffset[1]) << 48 |
                                                 static_cast<quint64>(ptrOffset[2]) << 40 |
                                                 static_cast<quint64>(ptrOffset[3]) << 32 |
                                                 static_cast<quint64>(ptrOffset[4]) << 24 |
                                                 static_cast<quint64>(ptrOffset[5]) << 16 |
                                                 static_cast<quint64>(ptrOffset[6]) << 8 |
                                                 static_cast<quint64>(ptrOffset[7]);

                                //通道值转换
                                channel = (channel == 0xFFF1) ? 1 : 2;

                                //粒子模式数据1024*8byte,前6字节:时间,后2字节:能量
                                int ref = 1;
                                ptrOffset += 8;

                                vector<long long> dataT;
                                vector<unsigned int> dataE;
                                while (ref++<=1024){
                                    long long t = static_cast<quint64>(ptrOffset[0]) << 40 |
                                            static_cast<quint64>(ptrOffset[1]) << 32 |
                                            static_cast<quint64>(ptrOffset[2]) << 24 |
                                            static_cast<quint64>(ptrOffset[3]) << 16 |
                                            static_cast<quint64>(ptrOffset[4]) << 8 |
                                            static_cast<quint64>(ptrOffset[5]);
                                    unsigned int e = static_cast<quint16>(ptrOffset[6]) << 8 | static_cast<quint16>(ptrOffset[7]);

                                    aStream << channel << t << e << "\n";
                                }
                            } else {
                                // 重新寻找帧头
                                while (ptrOffset != ptrTail){
                                    if (ptrOffset[framelen-4] == 0x00 && ptrOffset[framelen-3] == 0x00 && ptrOffset[framelen-2] == 0xaa && ptrOffset[framelen-1] == 0xb3){
                                        break;
                                    }

                                    ptrOffset++;
                                }

                                qint32 offset = ptrOffset - ptrHead;
                                memmove(buf, buf + offset, cachelen - offset);
                                cachelen -= offset;
                            }
                        } else {
                            // 寻找帧头
                            // 重新寻找帧头
                            while (ptrOffset != ptrTail){
                                if (ptrOffset[framelen-4] == 0x00 && ptrOffset[framelen-3] == 0x00 && ptrOffset[framelen-2] == 0xaa && ptrOffset[framelen-1] == 0xb3){
                                    break;
                                }

                                ptrOffset++;
                            }

                            qint32 offset = ptrOffset - ptrHead;
                            memmove(buf, buf + offset, cachelen - offset);
                            cachelen -= offset;
                        }

                        readlen = fileSrc.read((char*)buf+offset, framelen);
                        cachelen += readlen;
                    }

                    readlen = fileSrc.read((char*)buf+offset, framelen);
                }

                delete[] buf;
                fileDst.close();
            }

            fileSrc.close();
        }
    }
}

void CommandHelper::setDefaultCacheDir(QString dir)
{
    this->defaultCacheDir = dir;
}
