#include "commandhelper.h"
#include "sysutils.h"
#include <QDataStream>
#include <QApplication>
#include <QDateTime>
#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <iostream>

CommandHelper::CommandHelper(QObject *parent) : QObject(parent)
  , coincidenceAnalyzer(new CoincidenceAnalyzer)
{
    gainValue[0x00] = 0.08;
    gainValue[0x01] = 0.16;
    gainValue[0x02] = 0.32;
    gainValue[0x03] = 0.63;
    gainValue[0x04] = 1.26;
    gainValue[0x05] = 2.52;
    gainValue[0x06] = 5.01;
    gainValue[0x07] = 10.0;

    initCommand();//初始化常用指令
    initSocket(&socketRelay);
    //网络异常
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    connect(socketRelay, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [=](QAbstractSocket::SocketError){
        emit sigRelayFault();
    });
#else
    // Qt 5.15 及之后版本
    connect(socketRelay, &QAbstractSocket::errorOccurred,
            this, [=] { emit sigRelayFault(); });
#endif

    //状态改变
    connect(socketRelay, &QAbstractSocket::stateChanged, this, [=](QAbstractSocket::SocketState){
    });

    //连接成功
    connect(socketRelay, &QTcpSocket::connected, this, [=](){
        //发送查询指令
        if (socketRelay->property("relay-status").toString() == "query"){
            QByteArray command;
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
        } else if (socketRelay->property("relay-status").toString() == "open"){
            //发送开启指令
            QByteArray command;
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
    });

    //接收数据
    connect(socketRelay, &QTcpSocket::readyRead, this, [&](){
        QByteArray binaryData = socketRelay->readAll();

        // 判断指令只开关返回指令还是查询返回指令
        if (binaryData.size() == 15){
            if ((quint8)binaryData[0] == 0x48 && (quint8)binaryData[1] == 0x3a && (quint8)binaryData[2] == 0x01 && (quint8)binaryData[3] == 0x54){
                if ((quint8)binaryData[4] == 0x00 || (quint8)binaryData[5] == 0x00){
                    //继电器未开启
                    if (socketRelay->property("relay-status").toString() == "query"){
                        //socketRelay->setProperty("relay-status", "none");

                        //发送开启指令
                        QByteArray command;
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
                    } else{
                        // 如果已经发送过开启指令，则直接上报状态即可
                        emit sigRelayStatus(false);
                    }
                } else if ((quint8)binaryData[4] == 0x01 || (quint8)binaryData[5] == 0x01){
                    //已经开启
                    emit sigRelayStatus(true);
                }

                socketRelay->setProperty("relay-status", "none");
            }
        }
    });

    coincidenceAnalyzer->set_callback(analyzerRealCalback, this);

    connect(this, &CommandHelper::sigMeasureStop, this, [=](){
        //测量停止，进行符合计算，给出产额结果


        //测量停止保存符合运算测量参数
        QString configResultFile = validDataFileName + ".配置";
        {
            QFile file(configResultFile);
            if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
                QTextStream out(&file);

                if (detectorParameter.transferModel == 0x05){
                    // 保存粒子测量参数
                    out << tr("阈值1=") << detectorParameter.triggerThold1 << Qt::endl;
                    out << tr("阈值2=") << detectorParameter.triggerThold2 << Qt::endl;
                    out << tr("波形极性=") << ((detectorParameter.waveformPolarity==0x00) ? tr("正极性") : tr("负极性")) << Qt::endl;
                    out << tr("死时间=") << detectorParameter.deadTime << Qt::endl;
                    out << tr("波形触发模式=") << ((detectorParameter.triggerModel==0x00) ? tr("normal") : tr("auto")) << Qt::endl;
                    if (gainValue.contains(detectorParameter.gain))
                        out << tr("探测器增益=") << gainValue[detectorParameter.gain] << Qt::endl;
                    out << tr("量程选取=")<<detectorParameter.measureRange<< Qt::endl;
                    out << tr("冷却时长=") << detectorParameter.coolingTime << Qt::endl; //单位s
                    out << tr("符合延迟时间=") << detectorParameter.delayTime << Qt::endl; //单位ns
                    out << tr("符合分辨时间=") << detectorParameter.timeWidth << Qt::endl; //单位ns
                    out << tr("时间步长=") << 1 << Qt::endl; //注意，存储的数据时间步长恒为1s
                    out << tr("测量时长=") <<currentFPGATime<< Qt::endl;
                    out << tr("Det1符合能窗左=") << this->EnWindow[0] << Qt::endl;
                    out << tr("Det1符合能窗右=") << this->EnWindow[1] << Qt::endl;
                    out << tr("Det2符合能窗左=") << this->EnWindow[2] << Qt::endl;
                    out << tr("Det2符合能窗右=") << this->EnWindow[3] << Qt::endl;
                    if (detectorParameter.measureModel == mmManual){
                        //手动
                        out << tr("测量模式=手动") << Qt::endl;
                    } else if (detectorParameter.measureModel == mmAuto){
                        //自动
                        out << tr("测量模式=自动") << Qt::endl;
                    }
                }

                file.flush();
                file.close();
            }
        }
        qInfo() << tr("本次测量参数配置已存放在：%1").arg(configResultFile);
        
        //符合测量模式保存测量能谱文件
        QString SpecFile = validDataFileName + ".累积能谱";
        {
            QFile file(SpecFile);
            if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
                QTextStream out(&file);

                if (detectorParameter.transferModel == 0x05){
                    SingleSpectrum spec = coincidenceAnalyzer->GetAccumulateSpectrum();
                    out << "channel,"<< "SpectrumDet1,"<<"SpectrumDet2"<<Qt::endl;
                    for (size_t j=0; j<MULTI_CHANNEL; ++j){
                        out << j<<","<<spec.spectrum[0][j]<< ","<<spec.spectrum[1][j]<<Qt::endl;
                    }
                }
                file.flush();
                file.close();
            }
        }
        qInfo() << tr("本次测量累积能谱已存放在：%1").arg(SpecFile);
    });


    initSocket(&socketDetector);
    socketDetector->setSocketOption(QAbstractSocket::LowDelayOption, 1);//优化Socket以实现低延迟
    //网络异常
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    // Qt 5.15 之前版本
    connect(socketDetector, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [=](QAbstractSocket::SocketError){
        emit sigDetectorFault();});
#else
    // Qt 5.15 及之后版本
    connect(socketDetector, &QAbstractSocket::errorOccurred, this, [=] {
        // 网络故障，停止一切数据处理操作
        this->detectorException = true;
        emit sigDetectorFault();
    });
#endif

    //状态改变
    connect(socketDetector, &QAbstractSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
        if (state == QAbstractSocket::SocketState::UnconnectedState)
            emit sigDetectorStatus(false);
    });

    //连接成功
    connect(socketDetector, &QTcpSocket::connected, this, [=](){
        //发送位移指令
        emit sigDetectorStatus(true);
    });

    //接收数据
    connect(socketDetector, &QTcpSocket::readyRead, this, [=](){
        if (detectorParameter.measureModel == mmManual)
            handleManualMeasureNetData();
        else if (detectorParameter.measureModel == mmAuto)
            handleAutoMeasureNetData();
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

    closeSocket(socketRelay);
    closeSocket(socketDetector);

    delete coincidenceAnalyzer;
    coincidenceAnalyzer = nullptr;
}

void CommandHelper::analyzerRealCalback(SingleSpectrum r1, vector<CoincidenceResult> r3, void *callbackUser)
{
    CommandHelper *pThis = (CommandHelper*)callbackUser;
    pThis->doEnWindowData(r1, r3);
}

void CommandHelper::doEnWindowData(SingleSpectrum r1, vector<CoincidenceResult> r3)
{
    size_t count = r3.size();
    int _stepT = this->stepT;
    if (count <= 0)
        return;

    //保存信息
    if (r1.time != currentFPGATime){
        //有新的能谱数据产生
        QString coincidenceResultFile = validDataFileName + ".符合计数";
        {
            QFile::OpenMode ioFlags = QIODevice::Truncate;
            if (QFileInfo::exists(coincidenceResultFile))
                ioFlags = QIODevice::Append;
            QFile file(coincidenceResultFile);
            if (file.open(QIODevice::ReadWrite | QIODevice::Text | ioFlags)) {
                QTextStream out(&file);
                if (ioFlags == QIODevice::Truncate)
                    out << "time,CountRate1,CountRate2,ConCount_single,ConCount_multiple,deathRatio1,deathRatio2" << Qt::endl;
                CoincidenceResult coincidenceResult = r3.back();
                out << r1.time << "," << coincidenceResult.CountRate1 << "," << coincidenceResult.CountRate2 \
                    << "," << coincidenceResult.ConCount_single << "," << coincidenceResult.ConCount_multiple 
                    << "," << coincidenceResult.DeathRatio1 << "," << coincidenceResult.DeathRatio2 
                    << Qt::endl;

                file.flush();
                file.close();
            }
        }

        currentFPGATime = r1.time;
    }

    //时间步长，求均值
    if (_stepT > 1){
        if (count>1 && (count % _stepT == 0)){
            vector<CoincidenceResult> rr3;

            size_t posI = 0;
            for (size_t i=0; i < count/_stepT; i++){
                CoincidenceResult v;
                for (int j=0; j<_stepT; ++j){
                    v.CountRate1 += r3[posI].CountRate1;
                    v.CountRate2 += r3[posI].CountRate2;
                    v.ConCount_single += r3[posI].ConCount_single;
                    v.ConCount_multiple += r3[posI].ConCount_multiple;
                    posI++;
                }

                //给出平均计数率cps,注意，这里是整除，当计数率小于1cps时会变成零。
                v.time = r3[posI].time;
                v.CountRate1 /= _stepT;
                v.CountRate2 /= _stepT;
                v.ConCount_single /= _stepT;
                v.ConCount_multiple /= _stepT;
                
                rr3.push_back(v);
            }

            if (detectorParameter.measureModel == mmAuto)//自动测量，需要获取能宽
            {
                autoEnWindow.clear();
                coincidenceAnalyzer->GetEnWidth(autoEnWindow);
                emit sigUpdateAutoEnWidth(autoEnWindow);
            }

            emit sigPlot(r1, rr3);
        }
    } else{
        vector<CoincidenceResult> rr3;
        for (size_t i=0; i < count; i++){
            rr3.push_back(r3[i]);
        }

        if (detectorParameter.measureModel == mmAuto)//自动测量，需要获取能宽
        {
            autoEnWindow.clear();
            coincidenceAnalyzer->GetEnWidth(autoEnWindow);
            emit sigUpdateAutoEnWidth(autoEnWindow);
        }

        emit sigPlot(r1, rr3);
    }
}

void CommandHelper::initCommand()
{
    QDataStream dataStream(&cmdSoftTrigger, QIODevice::WriteOnly);
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
    dataStream << (quint8)((ch4 << 4) | (ch3 & 0x01));
    //是否开启相应通道,0关闭，1开启    CH2在高位，    CH1在低位
    dataStream << (quint8)((ch2 << 4) | (ch1 & 0x01));
    dataStream << (quint8)0x00;

    //00:停止 01:软件触发 02:硬件触发
    dataStream << (quint8)01;//0x01;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;

    cmdHardTrigger.append(cmdSoftTrigger);
    cmdHardTrigger[9] = 0x02;

    cmdStopTrigger.append(cmdSoftTrigger);
    // cmdStopTrigger[6] = 0x00;
    // cmdStopTrigger[7] = 0x00;
    cmdStopTrigger[9] = 0x00;

    //外触发信号
    cmdExternalTrigger.append((quint8)0x12);
    cmdExternalTrigger.append((quint8)0x34);
    cmdExternalTrigger.append((quint8)0x00);
    cmdExternalTrigger.append((quint8)0xAA);
    cmdExternalTrigger.append((quint8)0x00);
    cmdExternalTrigger.append((quint8)0x0C);
    cmdExternalTrigger.append((quint8)0x00);
    cmdExternalTrigger.append((quint8)0x00);
    cmdExternalTrigger.append((quint8)0x00);
    cmdExternalTrigger.append((quint8)0x00);
    cmdExternalTrigger.append((quint8)0xAB);
    cmdExternalTrigger.append((quint8)0xCD);
}

void CommandHelper::handleManualMeasureNetData()
{
    QByteArray binaryData = socketDetector->readAll();

#ifdef QT_DEBUG
    //粒子模式，DEBUG模式下保存原始数据查找问题
    if (nullptr != pfSaveNet && binaryData.size() > 0){
        pfSaveNet->write(binaryData);
        pfSaveNet->flush();
    }
#endif

    //00:能谱 03:波形 05:粒子
    if (workStatus == Preparing){
        QByteArray command = cmdPool.first();
        if (binaryData.compare(command) == 0){
            qDebug()<<"Recv HEX: "<<binaryData.toHex(' ');
            binaryData.remove(0, command.size());
            cmdPool.erase(cmdPool.begin());

            if (cmdPool.size() > 0)
            {
                socketDetector->write(cmdPool.first());
                qDebug()<<"Send HEX: "<<cmdPool.first().toHex(' ');
            }
            else{
                //最后指令是软件触发模式

                //测量已经开始了
                workStatus = Measuring;
                emit sigMeasureStart(detectorParameter.measureModel, detectorParameter.transferModel);
            }
        }
    }

    if (workStatus == Measuring && binaryData.size() > 0) {
        //符合模式需要过滤掉空值再写入到文件
        if (detectorParameter.transferModel != 0x05){
            if (nullptr != pfSaveNet && binaryData.size() > 0){
                pfSaveNet->write(binaryData);
                pfSaveNet->flush();
                emit sigRecvDataSize(binaryData.size());
            }
        }

        {
            QMutexLocker locker(&mutexCache);
            cachePool.push_back(binaryData);
        }
    }
}

void CommandHelper::handleAutoMeasureNetData()
{
    QByteArray binaryData = socketDetector->readAll();

#ifdef QT_DEBUG
    //粒子模式，DEBUG模式下保存原始数据查找问题
    if (nullptr != pfSaveNet && binaryData.size() > 0){
        pfSaveNet->write(binaryData);
        pfSaveNet->flush();
    }
#endif

    //00:能谱 03:波形 05:粒子
    if (workStatus == Preparing){
        QByteArray command = cmdPool.first();
        if (binaryData.compare(command) == 0){
            qDebug()<<"Recv HEX: "<<binaryData.toHex(' ');
            binaryData.remove(0, command.size());
            cmdPool.erase(cmdPool.begin());

            if (cmdPool.size() > 0)
            {
                socketDetector->write(cmdPool.first());
                socketDetector->waitForBytesWritten();
                qDebug()<<"Send HEX: "<<cmdPool.first().toHex(' ');
            }
            else{
                //最后指令是硬触发模式
                workStatus = Waiting;
                emit sigMeasureWait();

                //等待外触发指令了
            }
        }
    }

    if (workStatus == Waiting && binaryData.size() > 0){
        //等待硬件触发指令
        if (binaryData.compare(cmdExternalTrigger) == 0){
            qDebug()<<"Recv HEX: "<<binaryData.toHex(' ');
            qInfo()<<"接收到硬触发信号，探测器内部时钟开始计时";
            // 自动测量正式开始
            binaryData.remove(0, cmdExternalTrigger.size());

            workStatus = Measuring;
            emit sigMeasureStart(detectorParameter.measureModel, detectorParameter.transferModel);
        }
        // 还没等到触发但是直接停止了
        if (binaryData.compare(cmdStopTrigger) == 0){
            sigMeasureStop();
        }
    }

    if (workStatus == Measuring && binaryData.size() > 0) {
        // 只有符合模式才需要做进一步数据处理
        if (detectorParameter.transferModel == 0x05){
            QMutexLocker locker(&mutexCache);
            cachePool.push_back(binaryData);
        }
    }
}

void CommandHelper::startWork()
{
    // 创建数据解析线程
    analyzeNetDataThread = new QLiteThread(this);
    analyzeNetDataThread->setObjectName("analyzeNetDataThread");
    analyzeNetDataThread->setWorkThreadProc([=](){
        netFrameWorkThead();
    });
    analyzeNetDataThread->start();
    connect(this, &CommandHelper::destroyed, [=]() {
        analyzeNetDataThread->exit(0);
        analyzeNetDataThread->wait(500);
    });

    // 图形数据刷新
    plotUpdateThread = new QLiteThread(this);
    plotUpdateThread->setObjectName("plotUpdateThread");
    plotUpdateThread->setWorkThreadProc([=](){
        detTimeEnergyWorkThread();
    });
    plotUpdateThread->start();
    connect(this, &CommandHelper::destroyed, [=]() {
        plotUpdateThread->exit(0);
        plotUpdateThread->wait(500);
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

void CommandHelper::openRelay(bool first)
{
    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::information(nullptr, tr("提示"), tr("请先配置远程设备信息！"));
        return;
    }

    QString ip;
    qint32 port;
    // 读取文件内容
    QByteArray jsonData = file.readAll();
    file.close(); //释放资源

    // 将 JSON 数据解析为 QJsonDocument
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObj = jsonDoc.object();
    QJsonObject jsonDetector;
    if (jsonObj.contains("Relay")){
        jsonDetector = jsonObj["Relay"].toObject();
        ip = jsonDetector["ip"].toString();
        port = jsonDetector["port"].toInt();
    }

    //断开网络连接
    if (socketRelay->isOpen())
        socketRelay->disconnectFromHost();

    if (first)
        socketRelay->setProperty("relay-status", "query");
    else
        socketRelay->setProperty("relay-status", "open");
    socketRelay->connectToHost(ip, port);
}

void CommandHelper::closeRelay()
{
    QByteArray command;
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

    socketRelay->write(command);
}

void CommandHelper::openDetector()
{
    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::information(nullptr, tr("提示"), tr("请先配置远程设备信息！"));
        return;
    }

    // 读取文件内容
    QByteArray jsonData = file.readAll();
    file.close(); //释放资源

    // 将 JSON 数据解析为 QJsonDocument
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObj = jsonDoc.object();

    QString ip = "192.168.10.3";
    qint32 port = 5000;
    QJsonObject jsonDetector;
    if (jsonObj.contains("Detector")){
        jsonDetector = jsonObj["Detector"].toObject();
        ip = jsonDetector["ip"].toString();
        port = jsonDetector["port"].toInt();
    }

    //断开网络连接
    if (socketDetector->isOpen())
        socketDetector->disconnectFromHost();

    socketDetector->connectToHost(ip, port);
}

void CommandHelper::closeDetector()
{
    //停止测量
    slotStopManualMeasure();

    socketDetector->close();
    emit sigDetectorStatus(false);
}

//触发阈值1
QByteArray CommandHelper::getCmdTriggerThold1(quint16 ch1, quint16 ch2)
{
    QByteArray command;
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

    return command;
}

//波形极性
QByteArray CommandHelper::getCmdWaveformPolarity(quint8 v)
{
    QByteArray command;
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

    return command;
}

//波形触发模式
QByteArray CommandHelper::getCmdWaveformTriggerModel(quint8 mode)
{
    QByteArray command;
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

    return command;
}

//波形长度
QByteArray CommandHelper::getCmdWaveformLength(quint8 v)
{
    QByteArray command;
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

    return command;
}

//能谱模式/粒子模式死时间
QByteArray CommandHelper::getCmdDeadTime(quint16 deadTime)
{
    deadTime = deadTime/10;//转化为单位10ns

    QByteArray command;
    QDataStream dataStream(&command, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfe;
    dataStream << (quint8)0x16;

    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint16)deadTime;

    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;

    return command;
}

//能谱刷新时间
QByteArray CommandHelper::getCmdSpectnumRefreshTimeLength(quint32 refreshTimelength)
{
    QByteArray command;
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

    return command;
}

//探测器增益
QByteArray CommandHelper::getCmdDetectorGain(quint8 ch1, quint8 ch2, quint8 ch3, quint8 ch4)
{
    QByteArray command;
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

    return command;
}

//是否梯形成形
QByteArray CommandHelper::getCmdDetectorTS_flag(bool flag)
{
    QByteArray command;
    QDataStream dataStream(&command, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfc;
    dataStream << (quint8)0x10;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    if(flag) dataStream << (quint8)0x11; //开启梯形成形
    else dataStream << (quint8)0x00;//关闭梯形成形
    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;

    return command;
}

//梯形成形 上升沿、平顶、下降沿参数
QByteArray CommandHelper::getCmdDetectorTS_PointPara(quint8 rise, quint8 peak, quint8 fall)
{
    QByteArray command;
    QDataStream dataStream(&command, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfc;
    dataStream << (quint8)0x11;
    dataStream << (quint8)0x00;
    dataStream << (quint8)rise;
    dataStream << (quint8)peak;
    dataStream << (quint8)fall;
    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;

    return command;
}

//梯形成形 时间常数，time1,time2
QByteArray CommandHelper::getCmdDetectorTS_TimePara(quint16 time1, quint16 time2)
{
    QByteArray command;
    QDataStream dataStream(&command, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfc;
    dataStream << (quint8)0x12;
    dataStream << (quint16)time2;
    dataStream << (quint16)time1;
    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;

    return command;
}

//梯形成形 基线的噪声下限
QByteArray CommandHelper::getCmdDetectorTS_BaseLine(quint16 baseLineLowLimit)
{
    QByteArray command;
    QDataStream dataStream(&command, QIODevice::WriteOnly);
    dataStream << (quint8)0x12;
    dataStream << (quint8)0x34;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x0f;
    dataStream << (quint8)0xfc;
    dataStream << (quint8)0x13;
    dataStream << (quint8)0x00;
    dataStream << (quint8)0x00;
    dataStream << (quint16)baseLineLowLimit;
    dataStream << (quint8)0xab;
    dataStream << (quint8)0xcd;

    return command;
}

//传输模式
QByteArray CommandHelper::getCmdTransferModel(quint8 mode)
{
    QByteArray command;
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

    return command;
}

//开始手动测量
void CommandHelper::slotStartManualMeasure(DetectorParameter p)
{
    coincidenceAnalyzer->initialize();
    workStatus = Preparing;
    detectorParameter = p;
    this->autoChangeEnWindow = false;
    this->detectorException = false;
    sendStopCmd = false;

    //连接之前清空缓冲区
    QMutexLocker locker(&mutexCache);
    cachePool.clear();
    handlerPool.clear();

    QMutexLocker locker2(&mutexReset);
    currentSpectrumFrames.clear();

    //连接探测器
    prepareStep = 0;
    //文件夹准备
    QString ShotDir = defaultCacheDir + "/" + shotNum;
    QDir dir(ShotDir);
    if (!dir.exists())
        dir.mkdir(ShotDir);
    //创建缓存文件
    validDataFileName = QString("%1").arg(ShotDir + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd HHmmss") + "_valid.dat");   

#ifdef QT_DEBUG
    netDataFileName = QString("%1").arg(ShotDir + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd HHmmss") + "_Net.dat");
    if (nullptr != pfSaveNet){
        pfSaveNet->close();
        delete pfSaveNet;
        pfSaveNet = nullptr;
    }
    //网口数据缓存文件，波形模式、能谱模式直接存网口数据缓存文件
    pfSaveNet = new QFile(netDataFileName);
    if (pfSaveNet->open(QIODevice::WriteOnly)) {
        qInfo() << tr("创建网口数据缓存文件成功，文件名：%1").arg(netDataFileName);
    } else {
        qWarning() << tr("创建网口数据缓存文件失败，文件名：%1").arg(netDataFileName);
    }
#endif

    if (nullptr != pfSaveVaildData){
        pfSaveVaildData->close();
        delete pfSaveVaildData;
        pfSaveVaildData = nullptr;
    }
    
    //有效数据缓存文件。符合模式（也称粒子模式）只存有效数据
    pfSaveVaildData = new QFile(validDataFileName);
    if (pfSaveVaildData->open(QIODevice::WriteOnly)) {
        qInfo() << tr("创建缓存文件成功，文件名：%1").arg(validDataFileName);
    } else {
        qWarning() << tr("创建缓存文件失败，文件名：%1").arg(validDataFileName);
    }

    cmdPool.clear();
    if (detectorParameter.transferModel == 0x05){
        // 粒子模式

        //阈值
        cmdPool.push_back(getCmdTriggerThold1(detectorParameter.triggerThold1, detectorParameter.triggerThold2));
        //"设置波形极性
        cmdPool.push_back(getCmdWaveformPolarity(detectorParameter.waveformPolarity));
        //设置死时间
        cmdPool.push_back(getCmdDeadTime(detectorParameter.deadTime));
        // 探测器增益
        cmdPool.push_back(getCmdDetectorGain(detectorParameter.gain, detectorParameter.gain, 0x00, 0x00));
        // 传输模式
        cmdPool.push_back(getCmdTransferModel(detectorParameter.transferModel));
        //梯形成形
        if(detectorParameter.isTrapShaping) {
            //开启梯形成形
            cmdPool.push_back(getCmdDetectorTS_flag(true));
            // 梯形成形 上升沿、平顶、下降沿参数
            cmdPool.push_back(getCmdDetectorTS_PointPara(detectorParameter.TrapShape_risePoint,
                detectorParameter.TrapShape_peakPoint,
                detectorParameter.TrapShape_fallPoint));
            // 梯形成形 时间常数，time1,time2
            cmdPool.push_back(getCmdDetectorTS_TimePara(detectorParameter.TrapShape_constTime1,
                detectorParameter.TrapShape_constTime2));
            //基线的噪声下限
            cmdPool.push_back(getCmdDetectorTS_BaseLine(detectorParameter.TrapShape_baseLine));
        }
        else{
            //关闭梯形成形
            cmdPool.push_back(getCmdDetectorTS_flag(false));
            //基线的噪声下限
            cmdPool.push_back(getCmdDetectorTS_BaseLine(8140));
        }
        // 软触发模式
        cmdPool.push_back(cmdSoftTrigger);
    } else if (detectorParameter.transferModel == 0x00){
        //能谱模式

        //阈值
        cmdPool.push_back(getCmdTriggerThold1(detectorParameter.triggerThold1, detectorParameter.triggerThold2));
        //"设置波形极性
        cmdPool.push_back(getCmdWaveformPolarity(detectorParameter.waveformPolarity));
        //设置死时间
        cmdPool.push_back(getCmdDeadTime(detectorParameter.deadTime));
        //能谱刷新时间
        cmdPool.push_back(getCmdSpectnumRefreshTimeLength(detectorParameter.refreshTimeLength));
        // 探测器增益
        cmdPool.push_back(getCmdDetectorGain(detectorParameter.gain, detectorParameter.gain, 0x00, 0x00));
        // 传输模式
        cmdPool.push_back(getCmdTransferModel(detectorParameter.transferModel));
        //梯形成形
        if(detectorParameter.isTrapShaping) {
            //开启梯形成形
            cmdPool.push_back(getCmdDetectorTS_flag(true));
            // 梯形成形 上升沿、平顶、下降沿参数
            cmdPool.push_back(getCmdDetectorTS_PointPara(detectorParameter.TrapShape_risePoint, 
                detectorParameter.TrapShape_peakPoint,
                detectorParameter.TrapShape_fallPoint));
            // 梯形成形 时间常数，time1,time2
            cmdPool.push_back(getCmdDetectorTS_TimePara(detectorParameter.TrapShape_constTime1,
                detectorParameter.TrapShape_constTime2));
            //基线的噪声下限
            cmdPool.push_back(getCmdDetectorTS_BaseLine(detectorParameter.TrapShape_baseLine));
        }
        else{
            //关闭梯形成形
            cmdPool.push_back(getCmdDetectorTS_flag(false));
            //基线的噪声下限
            cmdPool.push_back(getCmdDetectorTS_BaseLine(8140));
        }
        // 开始测量，软触发模式
        cmdPool.push_back(cmdSoftTrigger);
    } else if (detectorParameter.transferModel == 0x03){
        // 波形模式

        //阈值
        cmdPool.push_back(getCmdTriggerThold1(detectorParameter.triggerThold1, detectorParameter.triggerThold2));
        //"设置波形极性
        cmdPool.push_back(getCmdWaveformPolarity(detectorParameter.waveformPolarity));
        // 探测器增益
        cmdPool.push_back(getCmdDetectorGain(detectorParameter.gain, detectorParameter.gain, 0x00, 0x00));
        // 波形长度
        cmdPool.push_back(getCmdWaveformLength(detectorParameter.waveLength));
        // 波形触发模式
        cmdPool.push_back(getCmdWaveformTriggerModel(detectorParameter.triggerModel));
        //梯形成形
        if(detectorParameter.isTrapShaping) {
            //开启梯形成形
            cmdPool.push_back(getCmdDetectorTS_flag(true));
            // 梯形成形 上升沿、平顶、下降沿参数
            cmdPool.push_back(getCmdDetectorTS_PointPara(detectorParameter.TrapShape_risePoint, 
                detectorParameter.TrapShape_peakPoint,
                detectorParameter.TrapShape_fallPoint));
            // 梯形成形 时间常数，time1,time2
            cmdPool.push_back(getCmdDetectorTS_TimePara(detectorParameter.TrapShape_constTime1,
                detectorParameter.TrapShape_constTime2));
            //基线的噪声下限
            cmdPool.push_back(getCmdDetectorTS_BaseLine(detectorParameter.TrapShape_baseLine));
        }
        else{
            //关闭梯形成形
            cmdPool.push_back(getCmdDetectorTS_flag(false));
            //基线的噪声下限
            cmdPool.push_back(getCmdDetectorTS_BaseLine(8140));
        }
        // 传输模式
        cmdPool.push_back(getCmdTransferModel(detectorParameter.transferModel));
        // 开始测量，软触发模式
        cmdPool.push_back(cmdSoftTrigger);
    }

    this->time_SetEnWindow = 0.0;
    socketDetector->write(cmdPool.first());
    qDebug()<<"Send HEX: "<<cmdPool.first().toHex(' ');
}

void CommandHelper::slotStopManualMeasure()
{
    if (nullptr == socketDetector)
        return;

    if (socketDetector->isWritable() && socketDetector->state() == QAbstractSocket::ConnectedState){
        cmdPool.push_back(cmdStopTrigger);
        socketDetector->write(cmdStopTrigger);
        sendStopCmd = true;
        qDebug()<<"Send HEX: "<<cmdStopTrigger.toHex(' ');
    } else {
        emit sigDetectorFault();
    }
}

//开始自动测量
void CommandHelper::slotStartAutoMeasure(DetectorParameter p)
{
    coincidenceAnalyzer->initialize();
    coincidenceAnalyzer->setCoolingTime_Auto(p.coolingTime);

    workStatus = Preparing;
    detectorParameter = p;
    sendStopCmd = false;
    this->detectorException = false;

    //连接之前清空缓冲区
    QMutexLocker locker(&mutexCache);
    cachePool.clear();
    handlerPool.clear();

    //连接探测器
    prepareStep = 0;
    //文件夹准备
    QString ShotDir = defaultCacheDir + "/" + shotNum;
    QDir dir(ShotDir);
    if (!dir.exists())
        dir.mkdir(ShotDir);
    //创建缓存文件
    validDataFileName = QString("%1").arg(ShotDir + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd HHmmss") + "_valid.dat");
#ifdef QT_DEBUG
    netDataFileName = QString("%1").arg(ShotDir + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd HHmmss") + "_Net.dat"); 
    if (nullptr != pfSaveNet){
        pfSaveNet->close();
        delete pfSaveNet;
        pfSaveNet = nullptr;
    }
    if (nullptr != pfSaveVaildData){
        pfSaveVaildData->close();
        delete pfSaveVaildData;
        pfSaveVaildData = nullptr;
    }

    //网口数据缓存文件，波形模式、能谱模式直接存网口数据缓存文件
    pfSaveNet = new QFile(netDataFileName);
    if (pfSaveNet->open(QIODevice::WriteOnly)) {
        qDebug() << tr("创建网口数据缓存文件成功，文件名：%1").arg(netDataFileName);
    } else {
        qWarning() << tr("创建网口数据缓存文件失败，文件名：%1").arg(netDataFileName);
    }
#endif

    //有效数据缓存文件。符合模式（也称粒子模式）只存有效数据
    pfSaveVaildData = new QFile(validDataFileName);
    if (pfSaveVaildData->open(QIODevice::WriteOnly)) {
        qDebug() << tr("创建缓存文件成功，文件名：%1").arg(validDataFileName);
    } else {
        qWarning() << tr("创建缓存文件失败，文件名：%1").arg(validDataFileName);
    }

    cmdPool.clear();
    //阈值
    cmdPool.push_back(getCmdTriggerThold1(detectorParameter.triggerThold1, detectorParameter.triggerThold2));
    // 波形极性
    cmdPool.push_back(getCmdWaveformPolarity(detectorParameter.waveformPolarity));
    // 死时间
    cmdPool.push_back(getCmdDeadTime(detectorParameter.deadTime));
    // 探测器增益
    cmdPool.push_back(getCmdDetectorGain(detectorParameter.gain, detectorParameter.gain, 0x00, 0x00));
    // 传输模式
    cmdPool.push_back(getCmdTransferModel(detectorParameter.transferModel));
    // 开始测量，硬触发模式
    cmdPool.push_back(cmdHardTrigger);

    socketDetector->write(cmdPool.first());
}

void CommandHelper::slotStopAutoMeasure()
{
    if (nullptr == socketDetector)
        return;

    cmdPool.push_back(cmdStopTrigger);
    socketDetector->write(cmdStopTrigger);
    sendStopCmd = true;
    qDebug()<<"Send HEX: "<<cmdStopTrigger.toHex(' ');
}

void CommandHelper::netFrameWorkThead()
{
    std::cout << "netFrameWorkThead thread id:" << QThread::currentThreadId() << std::endl;
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

        if (handlerPool.size() <= 0)
            continue;

        QByteArray validFrame;

        //00:能谱 03:波形 05:粒子
        if (detectorParameter.transferModel == 0x00){
            // 能谱个数
            //包头0x0000AAB2 + 能谱序号（32bit） + 测量时间（32bit） + 通道号（32bit）+ 能谱数据8187*32bit + 包尾0x0000CCD2

            if (1){
                // 直接写文件                
            } else {
                qint32 minPkgSize = 8192 * 4;
                bool isNual = false;
                while (true){
                    if (handlerPool.size() >= minPkgSize){
                        // 寻找包头
                        if ((quint8)handlerPool[0] == 0x00 && (quint8)handlerPool[1] == 0x00 && (quint8)handlerPool[2] == 0xaa && (quint8)handlerPool[3] == 0xb3){
                            // 寻找包尾(正常情况包尾正确)
                            if ((quint8)handlerPool[minPkgSize-4] == 0x00 && (quint8)handlerPool[minPkgSize-3] == 0x00 && (quint8)handlerPool[minPkgSize-2] == 0xcc && (quint8)handlerPool[minPkgSize-1] == 0xd3){
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
                }
            }
        } else if (detectorParameter.transferModel == 0x05)
        {
            const quint32 minPkgSize = (PARTICLE_NUM_ONE_PAKAGE + 1) * 16;//字节数
            bool isNual = false;
            bool foundStop = false;

            QDateTime tmStart = QDateTime::currentDateTime();

            //----------------------------------寻找包头包尾---------------------------------//
            int HeadIndex = -1; // 赋初值在0-258之外
            int TailIndex = -1;
            while (true){
                if(sendStopCmd) //且识别到已经发送停止指令，则可以检查是否读取到停止指令的返回。//当数据包小于minPkgSize，
                {
                    //12 34 00 0F FF 10 00 11 00 00 AB CD
                    //通过指令来判断测量是否停止
                    if (handlerPool.contains(cmdStopTrigger))
                    {
                        foundStop = true;
                        qDebug()<<"Recv HEX: "<<cmdStopTrigger.toHex(' ');
#ifdef QT_DEBUG
                        if (nullptr != pfSaveNet){
                            pfSaveNet->close();
                            delete pfSaveNet;
                            pfSaveNet = nullptr;
                        }
#endif
                        if (nullptr != pfSaveVaildData){
                            pfSaveVaildData->close();
                            delete pfSaveVaildData;
                            pfSaveVaildData = nullptr;
                        }
                    } else {
                        // handlerPool.clear();
                        // 因为有重新发送一次停止测量
                        // slotStopManualMeasure();
                        if(handlerPool.size()>12)  
                        {
                            int removeSize = handlerPool.size() - 12;
                            handlerPool = QByteArray(handlerPool.constData() + removeSize, handlerPool.size() - removeSize);
                        }
                        // sigMeasureStop();
                    }
                    break;
                }
                
                //对于大计数率下，网口数据非常大，处理不过来，直接清空缓存池，但是由于担心清空掉停止指令，所以补发一次停止指令。
                checkAndClearQByteArray(handlerPool);
                
                // 先尝试寻找完整数据包（数据包、指令包两类）
                quint32 size = handlerPool.size();
                if (size >= minPkgSize){
                    // DataHead 寻找包头 00 00 aa b3 
                    for (int i = 0; i < handlerPool.size() - 1; i++)
                    {
                        if ((quint8)handlerPool[i] == 0x00 && (quint8)handlerPool[i + 1] == 0x00
                            && (quint8)handlerPool[i + 2] == 0xaa && (quint8)handlerPool[i + 3] == 0xb3)
                        {
                            HeadIndex = i;
                            break;
                        }
                    }

                    // DataTail 寻找包尾 00 00 cc d3
                    // 有包头的情况下，才找包尾
                    if(HeadIndex>=0){
                        int posTail = HeadIndex + minPkgSize - 4;
                        // 直接尝试找到包尾
                        if ((quint8)handlerPool[posTail] == 0x00 && (quint8)handlerPool[posTail + 1] == 0x00
                            && (quint8)handlerPool[posTail + 2] == 0xcc && (quint8)handlerPool[posTail + 3] == 0xd3)
                        {
                            TailIndex = posTail;
                        }
                        else //标准位置不见包尾，找下一个头，说明这个数据包破损或者说异常
                        {
                            for (int i = 0; i < handlerPool.size() - 1; i++)
                            {
                                if ((quint8)handlerPool[i] == 0x00 && (quint8)handlerPool[i + 1] == 0x00
                                    && (quint8)handlerPool[i + 2] == 0xaa && (quint8)handlerPool[i + 3] == 0xb3)
                                {
                                    HeadIndex = i;
                                    break;
                                }
                            }
                        }
                    }
                }
                else{
                    break;
                }

                if ((HeadIndex == -1) || (TailIndex == -1)){ // 没找到包头包尾,并且包长度满足一包数据的最小值
                    if(!foundStop) //也没有找到停止指令
                    {                                                
                        //清空所有数据
                        handlerPool.clear();
                    }

                    break;
                }

                if (HeadIndex>-1 && HeadIndex > TailIndex) // 如果找到包头，且包头大于包尾则清除包头之前的数据
                {
                    //清空包头之前的数据
                    // handlerPool.remove(0, HeadIndex);
                    //使用指针重构（最高效但需谨慎）, 不移动数据，时间复杂度 O(1)
                    //handlerPool = QByteArray(handlerPool.constData() + HeadIndex, handlerPool.size() - HeadIndex);
                    char* data = handlerPool.data();
                    memmove(data, data + HeadIndex, handlerPool.size() - HeadIndex);
                    handlerPool.resize(handlerPool.size() - HeadIndex);
                    qDebug() << "发现坏包，数据长度:" << HeadIndex;
                    break;
                }

                if((TailIndex-HeadIndex) == (minPkgSize-4)){
                    isNual = true;
                    break;
                }
            }

            //发现停止指令
            if(foundStop) {
                //清空所有数据
                handlerPool.clear();
                workStatus = WorkEnd;
                emit sigMeasureStop();
            }

            //没有停止指令，且发现一个有效数据包
            if (isNual && (!foundStop)){
                //复制有效数据
                validFrame.append(handlerPool.data()+HeadIndex, minPkgSize);

                //使用指针重构（最高效但需谨慎）, 不移动数据，时间复杂度 O(1)
                //handlerPool = QByteArray(handlerPool.constData() + minPkgSize, handlerPool.size() - minPkgSize);
                char* data = handlerPool.data();
                memmove(data, data + minPkgSize, handlerPool.size() - minPkgSize);
                handlerPool.resize(handlerPool.size() - minPkgSize);

                //处理数据
                const unsigned char *ptrOffset = (const unsigned char *)validFrame.constData();

                //通道号(4字节)
                ptrOffset += 4; //包头4字节
                quint32 channel = static_cast<quint32>(ptrOffset[0]) << 24 |
                                  static_cast<quint32>(ptrOffset[1]) << 16 |
                                  static_cast<quint32>(ptrOffset[2]) << 8 |
                                  static_cast<quint32>(ptrOffset[3]);
                
                //序号（4字节）
                quint32 dataNum = static_cast<quint32>(ptrOffset[4]) << 24 |
                                  static_cast<quint32>(ptrOffset[5]) << 16 |
                                  static_cast<quint32>(ptrOffset[6]) << 8 |
                                  static_cast<quint32>(ptrOffset[7]);

                //通道值转换
                channel = (channel == 0xFFF1) ? 0 : 1;

                ptrOffset += 8;
                //粒子模式数据(PARTICLE_NUM_ONE_PAKAGE+1)*16byte,6字节:时间，2字节:死时间，2字节:幅度
                int ref = 1;
                std::vector<TimeEnergy> temp;
                while (ref++ <= PARTICLE_NUM_ONE_PAKAGE){
                    //空置48bit
                    ptrOffset += 6;

                    //时间:48bit
                    quint64 t = static_cast<quint64>(ptrOffset[0]) << 40 |
                                  static_cast<quint64>(ptrOffset[1]) << 32 |
                                  static_cast<quint64>(ptrOffset[2]) << 24 |
                                  static_cast<quint64>(ptrOffset[3]) << 16 |
                                  static_cast<quint64>(ptrOffset[4]) << 8 |
                                  static_cast<quint64>(ptrOffset[5]);
                    t *= 10;
                    ptrOffset += 6;

                    //死时间:16bit
                    unsigned short deathtime = static_cast<quint16>(ptrOffset[0]) << 8 | static_cast<quint16>(ptrOffset[1]);
                    deathtime *=10;
                    ptrOffset += 2;

                    //幅度:16bit
                    unsigned short amplitude = static_cast<quint16>(ptrOffset[0]) << 8 | static_cast<quint16>(ptrOffset[1]);
                    ptrOffset += 2;

                    if (t != 0x00 && amplitude != 0x00)
                        temp.push_back(TimeEnergy(t, deathtime, amplitude));
                }

                //数据分拣完毕
                if (temp.size() > 0){
                    QMutexLocker locker(&mutexPlot);
                    DetTimeEnergy detTimeEnergy;
                    detTimeEnergy.channel = channel;
                    detTimeEnergy.timeEnergy.swap(temp);
                    currentSpectrumFrames.push_back(detTimeEnergy);

                    // //将一个粒子的有效数据写入文件
                    // if (detectorParameter.transferModel == 0x05){
                    //     if (nullptr != pfSaveVaildData){
                    //         //有效数据对4字节
                    //         quint32 size = detTimeEnergy.timeEnergy.size();
                    //         pfSaveVaildData->write((char*)&size, sizeof(quint32));
                    //         //探测器编号0/1:1字节
                    //         pfSaveVaildData->write((char*)&detTimeEnergy.channel, sizeof(detTimeEnergy.channel));
                    //         //数据对:12字节对
                    //         pfSaveVaildData->write((char*)detTimeEnergy.timeEnergy.data(), sizeof(TimeEnergy)*size);
                    //         pfSaveVaildData->flush();
                    //     }
                    // }
                }

                QDateTime tmStop = QDateTime::currentDateTime();
                // qDebug() << "frame analyze time: " << tmStart.msecsTo(tmStop) << "ms";
            }
        } else if (detectorParameter.transferModel == 0x03){
            // 波形个数
            //包头0x0000AAB1 + 通道号（16bit） + 波形数据（4096*16bit） + 包尾0x0000CCD1
            // 使用示例
            //std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x01, 0x02, 0x03};
            //std::vector<uint8_t> target = {0x00, 0x00, 0xaa, 0x0b1};
            if (0x03 == detectorParameter.transferModel){
                // 波形个数
                //包头0x0000AAB1 + 通道号（16bit） + 波形数据（4096*16bit） + 包尾0x0000CCD1
                // 使用示例
                //std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x01, 0x02, 0x03};
                qint32 count = 0;
                std::vector<uint8_t> target = {0x00, 0x00, 0xaa, 0x0b1};
                while (true){
                    quint32 size = handlerPool.size();
                    quint32 minPkgSize = (4096+5) * 2;
                    if (size >= minPkgSize){
                        // 寻找包头
                        if ((quint8)handlerPool[0] == 0x00 && (quint8)handlerPool[1] == 0x00 && (quint8)handlerPool[2] == 0xaa && (quint8)handlerPool[3] == 0xb1){
                            // 寻找包尾(正常情况包尾正确)
                            if ((quint8)handlerPool[minPkgSize-4] == 0x00 && (quint8)handlerPool[minPkgSize-3] == 0x00 && (quint8)handlerPool[minPkgSize-2] == 0xcc && (quint8)handlerPool[minPkgSize-1] == 0xd1){
                                handlerPool.remove(0, minPkgSize);
                                count++;
                                continue;
                            } else {
                                handlerPool.remove(0, 1);
                            }     
                        } else {
                            handlerPool.remove(0, 1);
                        }
                    } else if (handlerPool.size() == 12){
                        if (handlerPool.compare(cmdStopTrigger) == 0){                            
                            qDebug()<<"Recv HEX: "<<cmdStopTrigger.toHex(' ');
#ifdef QT_DEBUG
                            if (nullptr != pfSaveNet){
                                pfSaveNet->close();
                                delete pfSaveNet;
                                pfSaveNet = nullptr;
                            }
#endif
                            //测量停止是否需要清空所有数据
                            handlerPool.clear();
                            workStatus = WorkEnd;
                            emit sigMeasureStop();
                            break;
                        } else {
                            handlerPool.remove(0, 1);
                        }
                    } else {
                        break;
                    }
                }

                if (0 != count)
                    emit sigRecvPkgCount(count);
            }
        } else {
            handlerPool.clear();
        }

        // QThread::msleep(1);
    }
}

void CommandHelper::detTimeEnergyWorkThread()
{
    std::cout << "detTimeEnergyWorkThread thread id:" << QThread::currentThreadId() << std::endl;
    QDateTime tmStart = QDateTime::currentDateTime().addDays(-1);
    vector<TimeEnergy> data1_2, data2_2;
    while (!taskFinished)
    {
        {
            QDateTime tmNow = QDateTime::currentDateTime();
            if (tmStart.msecsTo(tmNow) >= 500){
                tmStart = tmNow;
                std::vector<DetTimeEnergy> swapFrames;
                {
                    QMutexLocker locker(&mutexPlot);
                    swapFrames.swap(currentSpectrumFrames);
                }

                QMutexLocker locker(&mutexReset);
                //2、处理缓存区数据
                if (swapFrames.size() > 0){
                    for (size_t i=0; i<swapFrames.size(); ++i){
                        DetTimeEnergy detTimeEnergy = swapFrames[i];
                            
                        //有效数据写入文件，注意，对于自动测量，冷却时间之前的数据都保存了，但是不能做符合计算处理。
                        if (detectorParameter.transferModel == 0x05){
                            if (nullptr != pfSaveVaildData){
                                //有效数据对数目（也就是一个包里面有多少个粒子），4字节
                                quint32 size = detTimeEnergy.timeEnergy.size();
                                pfSaveVaildData->write((char*)&size, sizeof(quint32));
                                //探测器编号0/1:1字节
                                pfSaveVaildData->write((char*)&detTimeEnergy.channel, sizeof(detTimeEnergy.channel));
                                //数据对:12字节对
                                pfSaveVaildData->write((char*)detTimeEnergy.timeEnergy.data(), sizeof(TimeEnergy)*size);
                                pfSaveVaildData->flush();
                            }
                        }

                        // 根据步长，将数据添加到当前处理缓存
                        quint8 channel = detTimeEnergy.channel;
                        if (channel != 0x00 && channel != 0x01){
                            qDebug() << "error";
                        }

                        for (size_t i=0; i<detTimeEnergy.timeEnergy.size(); ++i){
                            TimeEnergy timeEnergy = detTimeEnergy.timeEnergy[i];

                            if (channel == 0x00){
                                data1_2.push_back(timeEnergy);
                            } else {
                                data2_2.push_back(timeEnergy);
                            }
                        }
                    }

                    if (data1_2.size() > 0 || data2_2.size() > 0 ){
                        QDateTime now = QDateTime::currentDateTime();
                        
                        if (detectorParameter.measureModel == mmAuto){//自动测量，需要获取能宽
                            //在冷却时长之后的数据才进行处理
                            if(data1_2.begin()->time/1e9 >= detectorParameter.coolingTime || data2_2.begin()->time/1e9 >= detectorParameter.coolingTime)
                            {
                                coincidenceAnalyzer->calculate(data1_2, data2_2, EnWindow, \
                                    detectorParameter.timeWidth, detectorParameter.delayTime, true, true);
                            }
                        }
                        else if(detectorParameter.measureModel == mmManual)
                        {
                            if (this->autoChangeEnWindow)
                            {
                                coincidenceAnalyzer->calculate(data1_2, data2_2, EnWindow, \
                                    detectorParameter.timeWidth, detectorParameter.delayTime, true, true);
                            }
                            else
                                coincidenceAnalyzer->calculate(data1_2, data2_2, EnWindow, \
                                    detectorParameter.timeWidth, detectorParameter.delayTime, true, false);
                        }
#ifdef QT_DEBUG
                    qDebug()<< "coincidenceAnalyzer->calculate time=" << now.msecsTo(QDateTime::currentDateTime()) \
                        << "ms, data1.count=" << data1_2.size() \
                        << ", data2.count=" << data2_2.size();
#endif
                        data1_2.clear();
                        data2_2.clear();
                    }
                }
            }
        }

        QThread::msleep(5);
    }
}

void CommandHelper::updateStepTime(int _stepT)
{
    QMutexLocker locker(&mutexReset);
    this->stepT = _stepT;
}

void CommandHelper::updateParamter(int _stepT, unsigned short _EnWin[4], bool _autoEnWindow)
{
    QMutexLocker locker(&mutexReset);

    this->stepT = _stepT;
    this->EnWindow[0] = _EnWin[0];
    this->EnWindow[1] = _EnWin[1];
    this->EnWindow[2] = _EnWin[2];
    this->EnWindow[3] = _EnWin[3];
    
    this->autoChangeEnWindow = _autoEnWindow;
    int count = coincidenceAnalyzer->GetCountCoin();
    // 如果已经处理了一段数据，或者是当前时刻大于0，则记录下此时的时刻
    if(count>0) this->time_SetEnWindow = coincidenceAnalyzer->GetPointPerSeconds().back().time;
}

void CommandHelper::switchToCountMode(bool _refModel)
{
    this->refModel = _refModel;
}

#include <QMessageBox>
void CommandHelper::exportFile(QString dstPath)
{
    if (dstPath.endsWith(".dat")){
        QFile file(dstPath);
        if (file.exists()) {
            file.remove();
        }

        //进行文件copy
        QFile::copy(validDataFileName + ".配置", dstPath + ".配置");
        if (this->detectorParameter.transferModel == 0x05){
            QFile::copy(validDataFileName + ".符合计数", dstPath + ".符合计数");
        }

        if (QFile::copy(validDataFileName, dstPath))        
            QMessageBox::information(nullptr, tr("提示"), tr("数据保存成功！"), QMessageBox::Ok, QMessageBox::Ok);
        else
            QMessageBox::information(nullptr, tr("提示"), tr("数据保存失败！"), QMessageBox::Ok, QMessageBox::Ok);

    } else if (dstPath.endsWith(".txt")) {
        // 将源文件转换为文本文件
        QFile fileSrc(validDataFileName);
        if (fileSrc.open(QIODevice::ReadOnly)) {
            QFile fileDst(dstPath);
            if (fileDst.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QDataStream aStream(&fileDst);

                qint32 cachelen = 0;
                qint32 framelen = (PARTICLE_NUM_ONE_PAKAGE + 1)* 16;
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
                                ptrOffset += 4; //包头4字节
                                //通道号 4字节
                                quint32 channel = static_cast<quint64>(ptrOffset[0]) << 24 |
                                                 static_cast<quint64>(ptrOffset[1]) << 16 |
                                                 static_cast<quint64>(ptrOffset[2]) << 8 |
                                                 static_cast<quint64>(ptrOffset[3]);
                                //通道值转换
                                channel = (channel == 0xFFF1) ? 1 : 2;
                                               
                                //数据包序号，4字节。两个探测器分别从1开始编号
                                ptrOffset += 4; //包头4字节
                                quint32 dataNum = static_cast<quint64>(ptrOffset[0]) << 24 |
                                                 static_cast<quint64>(ptrOffset[1]) << 16 |
                                                 static_cast<quint64>(ptrOffset[2]) << 8 |
                                                 static_cast<quint64>(ptrOffset[3]);

                                //粒子模式数据PARTICLE_NUM_ONE_PAKAGE*8byte, 6字节:时间,后2字节:能量
                                int ref = 1;
                                ptrOffset += 4;

                                while (ref++ <= PARTICLE_NUM_ONE_PAKAGE){
                                    //6字节空置
                                    ptrOffset += 6;
                                    //时间 6字节
                                    quint64 t = static_cast<quint64>(ptrOffset[0]) << 40 |
                                            static_cast<quint64>(ptrOffset[1]) << 32 |
                                            static_cast<quint64>(ptrOffset[2]) << 24 |
                                            static_cast<quint64>(ptrOffset[3]) << 16 |
                                            static_cast<quint64>(ptrOffset[4]) << 8 |
                                            static_cast<quint64>(ptrOffset[5]);
                                    //死时间 2字节
                                    quint16 deathTime = static_cast<quint16>(ptrOffset[6]) << 8 | static_cast<quint16>(ptrOffset[7]);
                                    //幅度 2字节
                                    quint16 amplitude = static_cast<quint16>(ptrOffset[8]) << 8 | static_cast<quint16>(ptrOffset[9]);
                                    aStream << channel << " "<< t <<" "<<" "<<deathTime<<" "<< amplitude << "\n";
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

bool CommandHelper::isConnected()
{
    //探测器是否连接
    return socketDetector->isOpen();
}

// 暂时弃用，没有调用该函数
void CommandHelper::analyzerCalback(SingleSpectrum r1, vector<CoincidenceResult> r3)
{
    size_t count = r3.size();
    int _stepT = this->stepT;
    
    if (count <= 0)
        return;

    QDateTime now = QDateTime::currentDateTime();
    std::cout << "[" << now.toString("hh:mm:ss.zzz").toStdString() \
              << "] coincidenceAnalyzer->calculate time=" << now.msecsTo(QDateTime::currentDateTime()) \
              << ", time=" << r1.time \
              << ", count=" << r3.size() << std::endl;
    return;

    //保存信息
    if (r1.time != currentFPGATime){
        //有新的能谱数据产生
        QString coincidenceResultFile = validDataFileName + ".符合计数";
        {
            QFile file(coincidenceResultFile);
            if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
                QTextStream out(&file);
                out << "time,CountRate1,CountRate2,ConCount_single,ConCount_multiple,deathRatio1,deathRatio2";
                CoincidenceResult coincidenceResult = r3.back();
                for (size_t i=0; i<r3.size(); ++i){
                    out << r1.time << "," << coincidenceResult.CountRate1 << "," << coincidenceResult.CountRate2 \
                        << "," << coincidenceResult.ConCount_single << "," << coincidenceResult.ConCount_multiple
                        << "," << coincidenceResult.DeathRatio1 << "," << coincidenceResult.DeathRatio2;
                }

                file.flush();
                file.close();
            }
        }
        currentFPGATime = r1.time;
    }

    //时间步长，求均值
    if (_stepT > 1){
        if (count>1 && (count % _stepT == 0)){
            vector<CoincidenceResult> rr3;

            for (size_t i=0; i < count/_stepT; i++){
                CoincidenceResult v;
                for (int j=0; j<_stepT; ++j){
                    size_t posI = i*_stepT + j;
                    //冷却时长内的数据才是有效数据
                    v.CountRate1 += r3[posI].CountRate1;
                    v.CountRate2 += r3[posI].CountRate2;
                    v.ConCount_single += r3[posI].ConCount_single;
                    v.ConCount_multiple += r3[posI].ConCount_multiple;
                }

                //给出平均计数率cps,注意，这里是整除，当计数率小于1cps时会变成零。
                v.CountRate1 /= _stepT;
                v.CountRate2 /= _stepT;
                v.ConCount_single /= _stepT;
                v.ConCount_multiple /= _stepT;
                rr3.push_back(v);
            }

            sigPlot(r1, rr3);
        }
    } else{
        vector<CoincidenceResult> rr3;
        for (size_t i=0; i < count; i++){
            rr3.push_back(r3[i]);
        }

        sigPlot(r1, rr3);
    }
}

bool CommandHelper::checkAndClearQByteArray(QByteArray &data) {
    if (data.capacity() > MAX_BYTEARRAY_SIZE) {
        qCritical() << "探测器计数率超出当前仪器计数率设计的上限，数据会发生丢失，导致计数率失真。\n"
                   <<"可能原因: 1)、放射性活度过高; 2)波形触发阈值过低导致。";
        data.clear();
        data.squeeze(); // 释放内存
        return true;
    }
    return false;
}
