#include "commandhelper.h"
#include <math.h>
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

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include <QNetworkSession>
#include <QNetworkConfigurationManager>

#include <QLockFile>
#include "globalsettings.h"
double g_enCalibration[2];
unsigned int multiChannel = 8192; //多道道数
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
    loadConfig(); //加载配置文件user.json

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
            if ((quint8)binaryData.at(0) == 0x48 && (quint8)binaryData.at(1) == 0x3a && (quint8)binaryData.at(2) == 0x01 && (quint8)binaryData.at(3) == 0x54){
                if ((quint8)binaryData.at(4) == 0x00 || (quint8)binaryData.at(5) == 0x00){
                    //继电器未开启
                    // if (socketRelay->property("relay-status").toString() == "query"){
                    //     //socketRelay->setProperty("relay-status", "none");

                    //     //发送开启指令
                    //     QByteArray command;
                    //     QDataStream dataStream(&command, QIODevice::WriteOnly);
                    //     dataStream << (quint8)0x48;
                    //     dataStream << (quint8)0x3a;
                    //     dataStream << (quint8)0x01;
                    //     dataStream << (quint8)0x57;
                    //     dataStream << (quint8)0x01;
                    //     dataStream << (quint8)0x01;
                    //     dataStream << (quint8)0x00;
                    //     dataStream << (quint8)0x00;
                    //     dataStream << (quint8)0x00;
                    //     dataStream << (quint8)0x00;
                    //     dataStream << (quint8)0x00;
                    //     dataStream << (quint8)0x00;
                    //     dataStream << (quint8)0xdc;
                    //     dataStream << (quint8)0x45;
                    //     dataStream << (quint8)0x44;
                    //     socketRelay->write(command);
                    // } else{
                    //     // 如果已经发送过开启指令，则直接上报状态即可
                    //     emit sigRelayStatus(false);
                    // }
                    emit sigRelayStatus(false);
                } else if ((quint8)binaryData.at(4) == 0x01 || (quint8)binaryData.at(5) == 0x01){
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
        if(detectorParameter.transferModel == 0x05){
            //测量停止保存测量时长，其他测量参数在开始测量时已经保存
            QString configResultFile = ShotDir + "/" + str_start_time + "_配置.txt";
            {
                QFile file(configResultFile);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
                    QTextStream out(&file);
                    out << tr("测量时长(s)=") <<currentFPGATime<< Qt::endl;
                    file.flush();
                    file.close();
                }
            }

            //记录FPGA丢包的数据，用以离线分析时的修正,该数据放在有效数据文件末尾
            QString lossDataFile = validDataFileName;
            {
                QFile file(validDataFileName);
                if (file.open(QIODevice::WriteOnly | QIODevice::Append)) { //二进制写入
                    if(!lossData.empty()){
                        quint32 mapSize = lossData.size();
                        unsigned char channel = 2;
                        file.write((char*)&mapSize, sizeof(quint32));
                        file.write((char*)&channel, sizeof(unsigned char));
                        vector<TimeEnergy> data3;
                        for (const auto& pair : lossData) {
                            TimeEnergy timeEn;
                            quint32 time = pair.first;
                            unsigned long long deltaTime = pair.second;
                            //这里将[deltaTime,time]转化为结构体TimeEnergy。读取时按照TimeEnergy来读取
                            timeEn.time = deltaTime;
                            timeEn.dietime = static_cast<unsigned short>(time >> 16); // 取高16位
                            timeEn.energy = static_cast<unsigned short>(time); //取低16位
                            data3.push_back(timeEn);
                        }
                        file.write((char*)data3.data(), sizeof(TimeEnergy)*mapSize);
                    }
                    file.flush();
                    file.close();
                }
            }
            // qDebug().noquote() << tr("本次测量FPGA丢包时刻及丢失时长已存放在：%1").arg(lossDataFile);

            //符合测量模式保存测量能谱文件
            QString SpecFile = ShotDir + "/" + str_start_time + "_累积能谱.txt";
            {
                vector<CoincidenceResult> coinresult = coincidenceAnalyzer->GetCoinResult();
                if(coinresult.size()>0) {
                    QFile file(SpecFile);
                    if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
                        QTextStream out(&file);

                        SingleSpectrum spec = coincidenceAnalyzer->GetAccumulateSpectrum();
                        out << "channel,"<< "SpectrumDet1,"<<"SpectrumDet2"<<Qt::endl;
                        for (size_t j=0; j<MULTI_CHANNEL; ++j){
                            out << j<<","<<spec.spectrum[0][j]<< ","<<spec.spectrum[1][j]<<Qt::endl;
                        }
                        file.flush();
                        file.close();
                    }
                }
            }
            qInfo().noquote() << tr("本次测量累积能谱已存放在：%1").arg(SpecFile);
        } else if(detectorParameter.transferModel == 0x03){
            //测量停止保存波形测量参数
            QString configResultFile = ShotDir + "/" + str_start_time + "_配置.txt";
            {
                QFile file(configResultFile);
                if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
                    QTextStream out(&file);

                    // 保存粒子测量参数
                    out << tr("阈值1=") << detectorParameter.triggerThold1 << Qt::endl;
                    out << tr("阈值2=") << detectorParameter.triggerThold2 << Qt::endl;
                    out << tr("波形极性=") << ((detectorParameter.waveformPolarity==0x00) ? tr("正极性") : tr("负极性")) << Qt::endl;

                    out << tr("波形触发模式=") << ((detectorParameter.triggerModel==0x00) ? tr("normal") : tr("auto")) << Qt::endl;
                    if (gainValue.contains(detectorParameter.gain))
                        out << tr("探测器增益=") << gainValue[detectorParameter.gain] << Qt::endl;

                    out << tr("波形长度=") << detectorParameter.waveLength << Qt::endl;

                    file.flush();
                    file.close();
                }
            }
            qInfo().noquote() << tr("本次波形测量参数配置已存放在：%1").arg(configResultFile);
        }
    });

    //异常终止，停止测量
    connect(this, &CommandHelper::sigAbonormalStop, this, [=](){
        if (nullptr != pfSaveNet){
            pfSaveNet->close();
            delete pfSaveNet;
            pfSaveNet = nullptr;
        }
    });


    connect(this, &CommandHelper::sigMeasureStopWave, this, [=](){
        //测量停止，进行符合计算，给出产额结果
        if(detectorParameter.transferModel == 0x03){
            //测量停止保存波形测量参数
            QString configResultFile = ShotDir + "/" + str_start_time + "_配置.txt";
            {
                QFile file(configResultFile);
                if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
                    QTextStream out(&file);

                    // 保存粒子测量参数
                    out << tr("阈值1=") << detectorParameter.triggerThold1 << Qt::endl;
                    out << tr("阈值2=") << detectorParameter.triggerThold2 << Qt::endl;
                    out << tr("波形极性=") << ((detectorParameter.waveformPolarity==0x00) ? tr("正极性") : tr("负极性")) << Qt::endl;

                    out << tr("波形触发模式=") << ((detectorParameter.triggerModel==0x00) ? tr("normal") : tr("auto")) << Qt::endl;
                    if (gainValue.contains(detectorParameter.gain))
                        out << tr("探测器增益=") << gainValue[detectorParameter.gain] << Qt::endl;

                    out << tr("波形长度=") << detectorParameter.waveLength << Qt::endl;

                    file.flush();
                    file.close();
                }
            }
            qInfo().noquote() << tr("本次波形测量参数配置已存放在：%1").arg(configResultFile);
        }
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
    connect(socketDetector, &QAbstractSocket::errorOccurred, this, [=](QAbstractSocket::SocketError error) {
        // 网络故障，停止一切数据处理操作
        if (error == QAbstractSocket::RemoteHostClosedError || error == QAbstractSocket::SocketTimeoutError){
            this->detectorException = true;
            emit sigDetectorFault();
        }
    });
#endif

    //状态改变
    connect(socketDetector, &QAbstractSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
        if (socketDetector->property("SocketState").toString() == "ConnectedState" && state == QAbstractSocket::SocketState::UnconnectedState){
            emit sigDetectorStatus(false);
            socketDetector->setProperty("SocketState", "UnconnectedState");
        }
    });

    //连接成功
    connect(socketDetector, &QTcpSocket::connected, this, [=](){
        //发送位移指令
        socketDetector->setProperty("SocketState", "ConnectedState");
        emit sigDetectorStatus(true);
    });

    //接收数据
    connect(socketDetector, &QTcpSocket::readyRead, this, [=](){
        if (detectorParameter.measureModel == mmManual)
            handleManualMeasureNetData();
        else if (detectorParameter.measureModel == mmAuto)
            handleAutoMeasureNetData();
    });

    //更改系统默认超时时长，让网络连接返回能够快点
    QNetworkConfigurationManager manager;
    QNetworkConfiguration config = manager.defaultConfiguration();
    QList<QNetworkConfiguration> cfg_list = manager.allConfigurations();
    if (cfg_list.size() > 0)
    {
        cfg_list.first().setConnectTimeout(1000);
        config = cfg_list.first();
    }
    QSharedPointer<QNetworkSession> spNetworkSession(new QNetworkSession(config));
    socketRelay->setProperty("_q_networksession", QVariant::fromValue(spNetworkSession));
    socketDetector->setProperty("_q_networksession", QVariant::fromValue(spNetworkSession));
}

CommandHelper::~CommandHelper(){
    taskFinished = true;

    //确保子线程退出
    ready = true;
    condition.wakeAll();

    analyzeNetDataThread->quit();
    analyzeNetDataThread->wait();
    
    plotUpdateThread->quit();
    plotUpdateThread->wait();

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

void CommandHelper::analyzerRealCalback(const SingleSpectrum &r1, const vector<CoincidenceResult> &r3, void *callbackUser)
{
    CommandHelper *pThis = (CommandHelper*)callbackUser;
    pThis->updateCoincidenceData(r1, r3);
}

void CommandHelper::loadConfig()
{
    JsonSettings* userSettings = GlobalSettings::instance()->mUserSettings;
    if (userSettings->isOpen()){
        userSettings->prepare();
        userSettings->beginGroup("Public");
        deltaTime_updateYield = userSettings->value("deltaTime_updateYield", 30).toInt();
        maxTime_updateYield = userSettings->value("maxTime_updateYield", 3600).toInt();
        userSettings->endGroup();
        userSettings->finish();
    } else{
        qCritical().noquote()<<"未找到拟合参数，请检查仪器是否进行了刻度,请对仪器刻度后重新计算（采用‘数据查看和分析’子界面分析）";
    }
}

void CommandHelper::updateCoincidenceData(const SingleSpectrum &r1, const vector<CoincidenceResult> &r3)
{
    size_t countCoin = r3.size();
    int _stepT = this->stepT;
    // if (countCoin <= 0) //对于手动测量，在选定能窗区域之前，只有能谱，没有符合计数曲线，这时countCoin=0
    //     return;

    //保存信息
    if (r1.time != currentFPGATime && r3.size()>0){
        //有新的能谱数据产生
        QString coincidenceResultFile = ShotDir + "/" + str_start_time + "_符合计数.txt";
        {
            QFile::OpenMode ioFlags = QIODevice::Truncate;
            if (QFileInfo::exists(coincidenceResultFile))
                ioFlags = QIODevice::Append;
            QFile file(coincidenceResultFile);
            if (file.open(QIODevice::ReadWrite | QIODevice::Text | ioFlags)) {
                QTextStream out(&file);
                if (ioFlags == QIODevice::Truncate)
                {
                    QString msg = "开始计数测量";
                    emit sigAppendMsg2(msg, QtInfoMsg);
                    out << "time,CountRate1,CountRate2,ConCount_single,ConCount_multiple,deathRatio1(%),deathRatio2(%)" << Qt::endl;
                }
                CoincidenceResult coincidenceResult = r3.back();
                out << coincidenceResult.time << "," << coincidenceResult.CountRate1 << "," << coincidenceResult.CountRate2 \
                    << "," << coincidenceResult.ConCount_single << "," << coincidenceResult.ConCount_multiple 
                    << "," << coincidenceResult.DeathRatio1 << "," << coincidenceResult.DeathRatio2 
                    << Qt::endl;

                file.flush();
                file.close();
            }
        }

        currentFPGATime = r1.time;
        currentTimeToShot = r3.back().time;
    }

    vector<CoincidenceResult> rr3;
    
    //对于自动测量，在第一次自动拟合之前的计数点数据不再显示，也不再作为中子产额计算处理。
    int start_time = 0;
    int start_pos = 0;
    vector<AutoGaussFit> gausslog = coincidenceAnalyzer->GetGaussFitLog();
    if(gausslog.size()>0 && detectorParameter.measureModel == mmAuto){
        start_time = gausslog.begin()->time;
        countCoin = r3.back().time - start_time;
        start_pos = r3.size() - countCoin;
    }

    if(countCoin >0 && countCoin <=maxTime_updateYield && countCoin%deltaTime_updateYield==0){
        //选取已测的全部数据点给出中子产额
        int start_time = r3.at(0).time - 1;
        int end_time = r3.back().time;
        //选取最近的一段时间给出中子产额
        // int start_time = r3.back().time - deltaTime_updateYield;
        // int end_time = r3.back().time;
        double At_omiga = coincidenceAnalyzer->getInintialActive(detectorParameter, start_time, end_time);
        activeOmigaToYield(At_omiga);
    }

    //时间步长，求均值
    //size_t posI = start_pos;
    if (_stepT > 1){
        if (countCoin>1 && (countCoin % _stepT == 0))
        {
            //对最近一个步长的计数数据求均值
            CoincidenceResult v;
            size_t posI = r3.size() - _stepT;
            for (size_t i=0; i < _stepT; i++){
                v.CountRate1 += r3.at(posI).CountRate1;
                v.CountRate2 += r3.at(posI).CountRate2;
                v.DeathRatio1 += r3.at(posI).DeathRatio1;
                v.DeathRatio2 += r3.at(posI).DeathRatio2;
                v.ConCount_single += r3.at(posI).ConCount_single;
                v.ConCount_multiple += r3.at(posI).ConCount_multiple;
                posI++;  
            }
            //给出平均计数率cps,注意，这里是整除，当计数率小于1cps时会变成零。
            v.time = r3.at(posI-1).time;
            v.CountRate1 /= _stepT;
            v.CountRate2 /= _stepT;
            v.ConCount_single /= _stepT;
            v.ConCount_multiple /= _stepT;
            emit sigPlot(r1, v);

            /*
                for (size_t i=0; i < countCoin/_stepT; i++){
                CoincidenceResult v;
                for (int j=0; j<_stepT; ++j){
                    v.CountRate1 += r3[posI].CountRate1;
                    v.CountRate2 += r3[posI].CountRate2;
                    v.ConCount_single += r3[posI].ConCount_single;
                    v.ConCount_multiple += r3[posI].ConCount_multiple;
                    posI++;
                }

                //给出平均计数率cps,注意，这里是整除，当计数率小于1cps时会变成零。
                v.time = r3[posI-1].time;
                v.CountRate1 /= _stepT;
                v.CountRate2 /= _stepT;
                v.ConCount_single /= _stepT;
                v.ConCount_multiple /= _stepT;
                
                rr3.push_back(v);
                emit sigPlot(r1, rr3);
            }
            */
        }
        else if(countCoin==0){//只绘制能谱
            sigNewPlot(r1, r3);
        }
    } else{
        // for (size_t i=0; i < countCoin; i++){
        //     rr3.push_back(r3[posI]);
        //     posI++;
        // }
        if(countCoin>1){
            emit sigPlot(r1, r3.back());
        }
        else{
            sigNewPlot(r1, r3);
        }
    }

    //更新能窗与能窗对应的竖线，目前默认都要更新，产生计数曲线后才会更新
    if(countCoin>0){
        std::vector<unsigned short> newEnWindow;
        coincidenceAnalyzer->GetEnWidth(newEnWindow);
        if(newEnWindow == autoEnWindow){
        }
        else{
            autoEnWindow = newEnWindow;
            std::vector<double> newAutoEnWindow;
            for (auto a : newEnWindow){
                newAutoEnWindow.push_back((double)a * g_enCalibration[0] + g_enCalibration[1]);
            }

            //打印能窗自动更新的日志
            vector<AutoGaussFit> gausslog = coincidenceAnalyzer->GetGaussFitLog();
            if(gausslog.size()>0){
                AutoGaussFit resultAutoGaussFit = gausslog.back();
                resultAutoGaussFit.EnLeft1 = resultAutoGaussFit.EnLeft1 * g_enCalibration[0] + g_enCalibration[1];
                resultAutoGaussFit.EnRight1 = resultAutoGaussFit.EnRight1 * g_enCalibration[0] + g_enCalibration[1];
                resultAutoGaussFit.EnLeft2 = resultAutoGaussFit.EnLeft2 * g_enCalibration[0] + g_enCalibration[1];
                resultAutoGaussFit.EnRight2 = resultAutoGaussFit.EnRight2 * g_enCalibration[0] + g_enCalibration[1];

                //有新的能窗产生
                QString autoEnChangeFile = ShotDir + "/" + str_start_time + "_能窗自动更新.txt";
                {
                    QFile::OpenMode ioFlags = QIODevice::Truncate;
                    if (QFileInfo::exists(autoEnChangeFile))
                        ioFlags = QIODevice::Append;
                    QFile file(autoEnChangeFile);
                    if (file.open(QIODevice::ReadWrite | QIODevice::Text | ioFlags)) {
                        QTextStream out(&file);
                        // if (ioFlags == QIODevice::Truncate)
                        // {
                        //     out << "time, Det1左能窗, Det1右能窗, Det2左能窗, Det2右能窗, Det1峰位, Det2峰位" << Qt::endl;
                        // }
                        out << resultAutoGaussFit.time << "," << resultAutoGaussFit.EnLeft1 << "," << resultAutoGaussFit.EnRight1 \
                            << "," << resultAutoGaussFit.EnLeft2 << "," << resultAutoGaussFit.EnRight2
                            << "," << resultAutoGaussFit.EnRight1 - resultAutoGaussFit.EnLeft1 << "," << resultAutoGaussFit.EnRight2 - resultAutoGaussFit.EnLeft2
                            << Qt::endl;

                        file.flush();
                        file.close();
                    }
                }
            }
            emit sigUpdateAutoEnWidth(newAutoEnWindow, detectorParameter.measureModel);
        }        
    }
}

#include <QSaveFile>
void CommandHelper::activeOmigaToYield(double active)
{
    //本次测量量程
    int measureRange = detectorParameter.measureRange - 1; //这里涉及到界面下拉框从0开始计数，而detParameter中从1开始计数。
    //读取配置文件，给出该量程下的刻度系数
    double cali_factor = 0.0;

    JsonSettings* userSettings = GlobalSettings::instance()->mUserSettings;
    if (userSettings->isOpen()){
        userSettings->prepare();
        userSettings->beginGroup("YieldCalibration");
        QString key = QString("Range%1").arg(measureRange);
        double yield = userSettings->arrayValue(key, 0, "Yield", measureRange==0 ? 10000 : (measureRange==1 ? 10000000 : 100000000000)).toDouble();
        double active0 = userSettings->arrayValue(key, 0, "active0", measureRange==0 ? 5000 : (measureRange==1 ? 6000 : 3333)).toDouble();

        cali_factor = yield / active0;
        userSettings->endGroup();
        userSettings->finish();
    } else{
        qCritical().noquote()<<"未找到拟合参数，请检查仪器是否进行了刻度,请对仪器刻度后重新计算（采用‘数据查看和分析’子界面分析）";
    }

    double result = active * cali_factor;

    //记录到文件中
    QString prefix = getFilenamePrefix();
    QString autoEnChangeFile = prefix + "_中子产额.txt";
    {
        QFile::OpenMode ioFlags = QIODevice::Truncate;
        if (QFileInfo::exists(autoEnChangeFile))
            ioFlags = QIODevice::Append;
        QFile file(autoEnChangeFile);
        if (file.open(QIODevice::ReadWrite | QIODevice::Text | ioFlags)) {
            QTextStream out(&file);
            if (ioFlags == QIODevice::Truncate)
            {
                out << tr("time(s), 相对活度, 中子产额") << Qt::endl;
            }
            out << getcurrentTimeToShot() << ","
                << QString::number(active, 'g', 9) << "," 
                <<QString::number(result, 'E', 6) << Qt::endl;

            file.flush();
            file.close();
        }
    }

    //更新界面，注意这里是子线程调用界面
    sigUpdate_Active_yield(active, result);
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

    //00:能谱 03:波形 05:粒子
    if (detectorParameter.transferModel == 0x03)
    {
        //波形模式，直接保存原始数据
        QMutexLocker locker(&mutexFile);
        if (nullptr != pfSaveNet && binaryData.size() > 0){
            pfSaveNet->write(binaryData);
            if(NetDataTimer.elapsed() > 1000) { // 每秒自动flush一次
                pfSaveNet->flush();
                NetDataTimer.restart();
            }
        }
        if(binaryData.size() > 0)
        {
            emit sigRecvDataSize(binaryData.size());
        }
    }
    else if(detectorParameter.transferModel == 0x05)
    {
        
        // 缓冲写入代替直接write
        m_netBuffer.append(binaryData);

        //粒子模式，DEBUG模式下保存原始数据查找问题
        // QMutexLocker locker(&mutexFile);
        if (nullptr != pfSaveNet){
            // 满足以下任一条件时flush:
            // 1. 缓冲区超过2MB
            // 2. 上次flush超过1秒
            if(m_netBuffer.size() > 2*1024*1024 || NetDataTimer.elapsed() > 1000)
            {
                pfSaveNet->write(m_netBuffer);
                pfSaveNet->flush();
                m_netBuffer.clear();
                NetDataTimer.restart();
                // qDebug().noquote()<<"Write net data";
            }
        }
    }

    //00:能谱 03:波形 05:粒子
    if (workStatus == Preparing){
        dataBuffer.append(binaryData);
        QByteArray command = cmdPool.first();
        bool found = false;
        
        // 检测接收数据中是否存在已发送的指令
        if (dataBuffer.size() >= 12) {
            //还没收到数据，停止测量
            if(sendStopCmd && binaryData.contains(cmdStopTrigger)){
                workStatus = WorkEnd;
                sigAbonormalStop();
                return;
            }

            // 查找目标数据包
            int pos = dataBuffer.indexOf(command);
            if (pos != -1) {
                found = true;
            }
        }

        if (found){
            qDebug()<<"Recv HEX: "<<dataBuffer.toHex(' ');
            
            // 从缓冲区移除已处理的数据
            dataBuffer.remove(0, command.size());
            cmdPool.erase(cmdPool.begin());

            if (cmdPool.size() > 0)
            {
                QThread::msleep(commandDelay);  // 延迟，毫秒（阻塞线程）
                socketDetector->write(cmdPool.first());
                qDebug()<<"Send HEX: "<<cmdPool.first().toHex(' ');
            }
            else{
                //最后指令是软件触发模式
                //测量已经开始了
                workStatus = Measuring;
                //将剩余的数据丢给后续处理
                if(dataBuffer.size()>0) cachePool.push_back(dataBuffer);
                emit sigMeasureStart(detectorParameter.measureModel, detectorParameter.transferModel);
            }
        }
        else
        {
            qDebug()<<"warning Recv HEX: "<<dataBuffer.toHex(' ');
        }
    }

    if (workStatus == Measuring && binaryData.size() > 0) {
        {
            QMutexLocker locker(&mutexCache);
            cachePool.push_back(binaryData);

            if (detectorParameter.measureModel == mmManual && detectorParameter.coolingTime == -1){
                QDateTime dtStart = QDateTime::currentDateTime();
                detectorParameter.coolingTime = detectorParameter.shotTime.secsTo(dtStart);
                coincidenceAnalyzer->setCoolingTime_Manual(detectorParameter.coolingTime);

                //收到第一帧有效数据再保存，保存测量参数
                saveConfigParamter();

                emit sigMeasureTimerStart(detectorParameter.measureModel, detectorParameter.transferModel, dtStart);
            }

            ready = true;
            condition.wakeAll();
        }
    }
}

void CommandHelper::handleAutoMeasureNetData()
{
    QByteArray binaryData = socketDetector->readAll();

// #ifdef QT_DEBUG
    //粒子模式，DEBUG模式下保存原始数据查找问题
    QMutexLocker locker(&mutexFile);
    if (nullptr != pfSaveNet && binaryData.size() > 0){
        pfSaveNet->write(binaryData);
        pfSaveNet->flush();
    }
// #endif

    //00:能谱 03:波形 05:粒子
    if (workStatus == Preparing){
        QByteArray command = cmdPool.first();
        if (binaryData.compare(command) == 0){
            qDebug()<<"Recv HEX: "<<binaryData.toHex(' ');
            binaryData.remove(0, command.size());
            cmdPool.erase(cmdPool.begin());

            if (cmdPool.size() > 0)
            {
                QThread::msleep(15);  // 延迟 30 毫秒（阻塞线程）
                socketDetector->write(cmdPool.first());
                // socketDetector->waitForBytesWritten();
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
            qInfo().noquote()<<"接收到硬触发信号，探测器内部时钟开始计时";
            // 自动测量正式开始
            binaryData.remove(0, cmdExternalTrigger.size());

            workStatus = Measuring;
            emit sigMeasureStart(detectorParameter.measureModel, detectorParameter.transferModel);
        }
        // 还没等到触发但是直接停止了
        if (binaryData.compare(cmdStopTrigger) == 0){
            sigAbonormalStop();
        }
    }

    if (workStatus == Measuring && binaryData.size() > 0) {
        // 只有符合模式才需要做进一步数据处理
        if (detectorParameter.transferModel == 0x05){
            QMutexLocker locker(&mutexCache);
            cachePool.push_back(binaryData);

            ready = true;
            condition.wakeAll();
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
    JsonSettings* ipSettings = GlobalSettings::instance()->mIpSettings;
    ipSettings->prepare();
    ipSettings->beginGroup("Relay");
    QString ip = ipSettings->value("ip").toString();
    qint32 port = ipSettings->value("port").toInt();
    ipSettings->endGroup();
    ipSettings->finish();

    //断开网络连接
    if (socketRelay->isOpen() && socketRelay->state() == QAbstractSocket::ConnectedState){
        if (socketRelay->peerAddress().toString() != ip || socketRelay->peerPort() != port)
            socketRelay->close();
        else{
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
            //emit sigRelayStatus(true);
            return;
        }
    }

    if (first)
        socketRelay->setProperty("relay-status", "query");
    else
        socketRelay->setProperty("relay-status", "open");
    socketRelay->connectToHost(ip, port);
    socketRelay->waitForConnected();
}

void CommandHelper::closeRelay()
{
    if (!socketRelay->isOpen()) return;

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

    if (pfSaveNet){
        pfSaveNet->close();
        pfSaveNet->deleteLater();
        pfSaveNet = nullptr;
    }

    if (pfSaveVaildData){
        pfSaveVaildData->close();
        pfSaveVaildData->deleteLater();
        pfSaveVaildData = nullptr;
    }
}

void CommandHelper::openDetector()
{
    JsonSettings* ipSettings = GlobalSettings::instance()->mIpSettings;
    ipSettings->prepare();
    ipSettings->beginGroup("Detector");
    QString ip = ipSettings->value("ip", "192.168.10.3").toString();
    qint32 port = ipSettings->value("port", 5000).toInt();
    ipSettings->endGroup();
    ipSettings->finish();

    if (socketDetector->state() == QAbstractSocket::ConnectingState){// 正在连接，请稍后
        QMessageBox::information(nullptr, tr("提示"), tr("正在连接，请稍后重试！"));
        return;
    }

    //断开网络连接
    if (socketDetector->isOpen() && socketDetector->state() == QAbstractSocket::ConnectedState){
        if (socketDetector->peerAddress().toString() != ip || socketDetector->peerPort() != port)
            socketDetector->close();
        else{
            sigDetectorStatus(true);
            return;
        }
    }

    socketDetector->connectToHost(ip, port);
    socketDetector->waitForConnected();
}

void CommandHelper::closeDetector()
{
    //停止测量
    slotStopManualMeasure();
    if (socketDetector->isOpen() && socketDetector->state() == QAbstractSocket::ConnectedState)
        socketDetector->disconnectFromHost();
    // emit sigDetectorStatus(false);
}

//触发阈值，设置通道1和通道2的触发阈值，通道3、通道4硬件上已经禁用了。
QByteArray CommandHelper::getCmdTriggerThold(quint16 ch1, quint16 ch2)
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
    JsonSettings* mUserSettings = GlobalSettings::instance()->mUserSettings;
    if (mUserSettings->isOpen())
    {
        mUserSettings->prepare();
        mUserSettings->beginGroup();
        g_enCalibration[0] = mUserSettings->value("EnCalibrration_k", 1.0).toDouble();
        g_enCalibration[1] = mUserSettings->value("EnCalibrration_b", 0.0).toDouble();
        mUserSettings->endGroup();
        mUserSettings->finish();

        if (g_enCalibration[0] < 0.0001)
            g_enCalibration[0] = 1.0;
    }

    coincidenceAnalyzer->initialize();
    //coincidenceAnalyzer->setCoolingTime_Manual(p.coolingTime);//此时冷却时间还未计算出来呢
    workStatus = Preparing;
    detectorParameter = p;
	detectorParameter.coolingTime = -1;
    this->autoChangeEnWindow = false;
    this->detectorException = false;
    sendStopCmd = false;
    startSaveValidData = false;
    autoEnWindow.clear();
    NetDataTimer.start();

    //先清空TCP发送区
    socketDetector->flush();  // 强制尝试发送（不阻塞，实际发送由系统调度）
    socketDetector->waitForBytesWritten();//等待数据发送完成。（阻塞直到数据写入操作系统）

    //再清空TCP接收区缓存，以及相应的缓存变量
    socketDetector->readAll();

    //连接之前清空缓冲区
    dataBuffer.clear();
    {
        QMutexLocker locker(&mutexCache);
        cachePool.clear();
        ready = true;
        condition.wakeAll();
    }
    handlerPool.clear();

    {
        QMutexLocker locker2(&mutexReset);
        currentSpectrumFrames.clear();
    }

    //连接探测器
    prepareStep = 0;
    SequenceNumber = 0;
    lossData.clear();

    //文件夹准备
    ShotDir = defaultCacheDir + "/" + shotNum;
    QDir dir(ShotDir);
    if (!dir.exists())
        dir.mkdir(ShotDir);
    
    if(detectorParameter.transferModel == 0x05)
    {
        {
            //先确保上一次的文件关闭
            QMutexLocker locker(&mutexFile);
            if (nullptr != pfSaveNet){
                pfSaveNet->close();
                delete pfSaveNet;
                pfSaveNet = nullptr;
            }
        }

        //创建网口数据缓存文件
        str_start_time = QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss");
        netDataFileName = QString("%1").arg(ShotDir + "/" + str_start_time + "_Net.dat");
        pfSaveNet = new QFile(netDataFileName);
        if (pfSaveNet->open(QIODevice::WriteOnly)) {
            qDebug().noquote() << tr("创建网口数据缓存文件成功，文件名：%1").arg(netDataFileName);
        } else {
            qDebug().noquote() << tr("创建网口数据缓存文件失败，文件名：%1").arg(netDataFileName);
        }

        if (nullptr != pfSaveVaildData){
            pfSaveVaildData->close();
            delete pfSaveVaildData;
            pfSaveVaildData = nullptr;
        }

        //创建缓存文件
        validDataFileName = QString("%1").arg(ShotDir + "/" + str_start_time + "_valid.dat");
        //有效数据缓存文件。符合模式（也称粒子模式）只存有效数据
        pfSaveVaildData = new QFile(validDataFileName);
        if (pfSaveVaildData->open(QIODevice::WriteOnly)) {
            unsigned int FileHead = 0xFFFFFFFF; //文件包头，有效数据的识别码
            qint64 num = pfSaveVaildData->write((char*)&FileHead, sizeof(FileHead));
            qInfo().noquote() << tr("创建缓存文件成功，文件名：%1").arg(validDataFileName);
            if(num == -1){
                qInfo().noquote() << tr("文件包头0xFFFFFFFF写入失败，文件名：%1").arg(validDataFileName);
            }
        } else {
            qWarning().noquote() << tr("创建缓存文件失败，文件名：%1").arg(validDataFileName);
        }
    }
    else if(detectorParameter.transferModel == 0x03)
    {
        {
            //关闭上一次可能未释放的文件
            QMutexLocker locker(&mutexFile);
            if (nullptr != pfSaveNet){
                pfSaveNet->close();
                delete pfSaveNet;
                pfSaveNet = nullptr;
            }
        }

        //创建网口数据缓存文件
        str_start_time = QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss");
        netDataFileName = QString("%1").arg(ShotDir + "/" + str_start_time + "_WaveNet.dat");
        pfSaveNet = new QFile(netDataFileName);
        if (pfSaveNet->open(QIODevice::WriteOnly)) {
            qInfo().noquote() << tr("创建网口数据缓存文件成功，文件名：%1").arg(netDataFileName);
        } else {
            qWarning().noquote() << tr("创建网口数据缓存文件失败，文件名：%1").arg(netDataFileName);
        }
    }

    cmdPool.clear();
    if (detectorParameter.transferModel == 0x05){
        // 粒子模式

        //阈值
        cmdPool.push_back(getCmdTriggerThold(detectorParameter.triggerThold1, detectorParameter.triggerThold2));
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
            cmdPool.push_back(getCmdDetectorTS_BaseLine(detectorParameter.Threshold_baseLine));
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
        cmdPool.push_back(getCmdTriggerThold(detectorParameter.triggerThold1, detectorParameter.triggerThold2));
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
            cmdPool.push_back(getCmdDetectorTS_BaseLine(detectorParameter.Threshold_baseLine));
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
        cmdPool.push_back(getCmdTriggerThold(detectorParameter.triggerThold1, detectorParameter.triggerThold2));
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
            cmdPool.push_back(getCmdDetectorTS_BaseLine(detectorParameter.Threshold_baseLine));
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
        cmdPool.clear();
        cmdPool.push_back(cmdStopTrigger);
        {
            //QMutexLocker locker(&mutexCache);
            sendStopCmd = true;
        }
        socketDetector->write(cmdStopTrigger);
        socketDetector->flush();
        socketDetector->waitForBytesWritten();
        qDebug()<<"Send HEX: "<<cmdStopTrigger.toHex(' ');
    } else {
        emit sigDetectorFault();
    }
}

//开始自动测量
void CommandHelper::setAutoMeasureParameter(DetectorParameter p)
{
    // 设置自动测量参数
    detectorParameter = p;
}

void CommandHelper::slotStartAutoMeasure()
{
    slotStartAutoMeasure(detectorParameter);
}

void CommandHelper::slotStartAutoMeasure(DetectorParameter p)
{
    JsonSettings* mUserSettings = GlobalSettings::instance()->mUserSettings;
    if (mUserSettings->isOpen())
    {
        mUserSettings->prepare();
        mUserSettings->beginGroup();
        g_enCalibration[0] = mUserSettings->value("EnCalibrration_k", 1.0).toDouble();
        g_enCalibration[1] = mUserSettings->value("EnCalibrration_b", 0.0).toDouble();
        mUserSettings->endGroup();
        mUserSettings->finish();

        if (g_enCalibration[0] < 0.0001)
            g_enCalibration[0] = 1.0;
    }

    coincidenceAnalyzer->initialize();
    coincidenceAnalyzer->setCoolingTime_Auto(p.coolingTime);

    workStatus = Preparing;
    detectorParameter = p;
    sendStopCmd = false;
    this->detectorException = false;

    //先清空TCP发送区
    socketDetector->flush();  // 强制尝试发送（不阻塞，实际发送由系统调度）
    socketDetector->waitForBytesWritten();//等待数据发送完成。（阻塞直到数据写入操作系统）

    //先清空TCP接收区缓存，以及相应的缓存变量
    socketDetector->readAll();

    //连接之前清空缓冲区
    {
        QMutexLocker locker(&mutexCache);
        cachePool.clear();
        ready = true;
        condition.wakeAll();
    }

    handlerPool.clear();

    //连接探测器
    prepareStep = 0;
    SequenceNumber = 0;
    lossData.clear();

    //文件夹准备
    ShotDir = defaultCacheDir + "/" + shotNum;
    QDir dir(ShotDir);
    if (!dir.exists())
        dir.mkdir(ShotDir);
    //创建缓存文件
    str_start_time = QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss");
    validDataFileName = QString("%1").arg(ShotDir + "/" + str_start_time + "_valid.dat");
    QMutexLocker locker2(&mutexFile);
// #ifdef QT_DEBUG
    netDataFileName = QString("%1").arg(ShotDir + "/" + str_start_time + "_Net.dat");
    if (nullptr != pfSaveNet){
        pfSaveNet->close();
        delete pfSaveNet;
        pfSaveNet = nullptr;
    }

    //网口数据缓存文件，波形模式、能谱模式直接存网口数据缓存文件
    pfSaveNet = new QFile(netDataFileName);
    if (pfSaveNet->open(QIODevice::WriteOnly)) {
        qDebug().noquote() << tr("创建网口数据缓存文件成功，文件名：%1").arg(netDataFileName);
    } else {
        qDebug().noquote() << tr("创建网口数据缓存文件失败，文件名：%1").arg(netDataFileName);
    }
// #endif

    if (nullptr != pfSaveVaildData){
        pfSaveVaildData->close();
        delete pfSaveVaildData;
        pfSaveVaildData = nullptr;
    }

    //有效数据缓存文件。符合模式（也称粒子模式）只存有效数据
    pfSaveVaildData = new QFile(validDataFileName);
    if (pfSaveVaildData->open(QIODevice::WriteOnly)) {
        unsigned int FileHead = 0xFFFFFFFF; //文件包头，有效数据的识别码
        pfSaveVaildData->write((char*)&FileHead, sizeof(FileHead));
        qInfo().noquote() << tr("创建缓存文件成功，文件名：%1").arg(validDataFileName);
    } else {
        qWarning().noquote() << tr("创建缓存文件失败，文件名：%1").arg(validDataFileName);
    }

    //保存测量参数,自动测量一开始就保存参数
    //saveConfigParamter();//这个时候冷却时间还没计算出来，等收到第一帧有效数据再保存

    cmdPool.clear();
    //阈值
    cmdPool.push_back(getCmdTriggerThold(detectorParameter.triggerThold1, detectorParameter.triggerThold2));
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
    qDebug()<<"Send HEX: "<<cmdPool.first().toHex(' ');
}

void CommandHelper::saveConfigParamter(){
    QString configResultFile = ShotDir + "/" + str_start_time + "_配置.txt";
    {
        QFile file(configResultFile);
        if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QTextStream out(&file);

            // 保存粒子测量参数
            out << tr("软件版本号=") << tr("Commit: %1").arg(APP_VERSION)<< Qt::endl;
            out << tr("测量开始时刻=") << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz ")<< Qt::endl;
            out << tr("阈值1=") << detectorParameter.triggerThold1 << Qt::endl;
            out << tr("阈值2=") << detectorParameter.triggerThold2 << Qt::endl;
            out << tr("波形极性=") << ((detectorParameter.waveformPolarity==0x00) ? tr("正极性") : tr("负极性")) << Qt::endl;
            out << tr("死时间(ns)=") << detectorParameter.deadTime << Qt::endl;
            out << tr("波形触发模式=") << ((detectorParameter.triggerModel==0x00) ? tr("normal") : tr("auto")) << Qt::endl;
            if (gainValue.contains(detectorParameter.gain))
                out << tr("探测器增益=") << gainValue[detectorParameter.gain] << Qt::endl;
            switch (detectorParameter.measureRange){
                case 1:
                    out << tr("量程选取=小量程")<< Qt::endl;
                    break;
                case 2:
                    out << tr("量程选取=中量程")<< Qt::endl;
                    break;
                case 3:
                    out << tr("量程选取=大量程")<< Qt::endl;
                    break;
                default:
                break;
            }
            out << tr("冷却时长(s)=") << detectorParameter.coolingTime << Qt::endl; //单位s
            
            //开始保存FPGA数据的时刻，单位s，以活化后开始计时(冷却时间+FPGA时钟)。
            int savetime_FPGA = 0;
            if(detectorParameter.measureModel == mmManual){
                savetime_FPGA = this->time_SetEnWindow;
            }else if(detectorParameter.measureModel == mmAuto){
                savetime_FPGA = detectorParameter.coolingTime;
            }

            out << tr("测量开始时间(冷却时间+FPGA时钟)=")<< savetime_FPGA <<Qt::endl;
            out << tr("符合延迟时间(ns))=") << detectorParameter.delayTime << Qt::endl; //单位ns
            out << tr("符合分辨时间(ns)=") << detectorParameter.timeWidth << Qt::endl; //单位ns
            out << tr("时间步长(s)=") << 1 << Qt::endl; //注意，存储的数据时间步长恒为1s
            // out << tr("测量时长(s)=") <<currentFPGATime<< Qt::endl;
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

            file.flush();
            file.close();
        }
    }
    qInfo().noquote() << tr("本次测量参数配置已存放在：%1").arg(configResultFile);

    //存放自动更新能窗的日志，存放第一次能窗数据，由于第一次能窗不一定是高斯拟合给出，因为不给出峰位
    QString autoEnChangeFile = ShotDir + "/" + str_start_time + "_能窗自动更新.txt";
    {
        QFile::OpenMode ioFlags = QIODevice::Truncate;
        if (QFileInfo::exists(autoEnChangeFile))
            ioFlags = QIODevice::Append;
        QFile file(autoEnChangeFile);
        if (file.open(QIODevice::ReadWrite | QIODevice::Text | ioFlags)) {
            QTextStream out(&file);
            if (ioFlags == QIODevice::Truncate)
            {   
                out << tr("time(s), Det1左能窗, Det1右能窗, Det2左能窗, Det2右能窗, Det1峰位, Det2峰位") << Qt::endl;
            }            
            out << detectorParameter.coolingTime << "," << this->EnWindow[0] << "," << this->EnWindow[1] \
                << "," << this->EnWindow[2] << "," << this->EnWindow[3]
                << Qt::endl;

            file.flush();
            file.close();
        }
    }
    qInfo().noquote() << tr("本次测量自动更新能窗的日志存放在：%1").arg(autoEnChangeFile);
}

void CommandHelper::slotStopAutoMeasure()
{
    if (nullptr == socketDetector)
        return;

    cmdPool.clear();
    cmdPool.push_back(cmdStopTrigger);
    {
        //QMutexLocker locker(&mutexCache);
        sendStopCmd = true;
    }
    socketDetector->write(cmdStopTrigger);
    socketDetector->flush();
    socketDetector->waitForBytesWritten();
    qDebug()<<"Send HEX: "<<cmdStopTrigger.toHex(' ');
}

void CommandHelper::netFrameWorkThead()
{
    std::cout << "netFrameWorkThead thread id:" << QThread::currentThreadId() << std::endl;
    while (!taskFinished)
    {
        //方案一
        {
            QMutexLocker locker(&mutexCache);
            if (cachePool.size() == 0){
                if (handlerPool.size() < 12){
                    // 数据单位最小值为12（一个指令长度）
                    while(!ready)
                    {
                        condition.wait(&mutexCache);
                    }

                    handlerPool.append(cachePool);
                    cachePool.clear();
                    ready = false;
                }
            } else {
                handlerPool.append(cachePool);
                cachePool.clear();
            }
        }

        if (handlerPool.size() == 0)
            continue;

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
                        if ((quint8)handlerPool.at(0) == 0x00 && (quint8)handlerPool.at(1) == 0x00 && (quint8)handlerPool.at(2) == 0xaa && (quint8)handlerPool.at(3) == 0xb3){
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
                    QByteArray validFrame(handlerPool.constData(), minPkgSize);
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
            
            //对于大计数率下，网口数据非常大，处理不过来，直接清空缓存池，但是由于担心清空掉停止指令，所以补发一次停止指令。
            checkAndClearQByteArray(handlerPool);
            int startPos = 0; 
            while (true){
                int HeadIndex = -1; // 赋初值在0-258之外
                int TailIndex = -1;
                if(sendStopCmd) //且识别到已经发送停止指令，则可以检查是否读取到停止指令的返回。//当数据包小于minPkgSize，
                {
                    //12 34 00 0F FF 10 00 11 00 00 AB CD
                    //通过指令来判断测量是否停止
                    if (handlerPool.contains(cmdStopTrigger))
                    {
                        foundStop = true;
                        qDebug()<<"Recv HEX: "<<cmdStopTrigger.toHex(' ');

                        QMutexLocker locker(&mutexFile);
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
                    } else {
                        // DataHead 寻找包头 12 34
                        for (startPos; startPos < handlerPool.size() - 2; startPos++)
                        {
                            if ((quint8)handlerPool.at(startPos) == 0x12 && (quint8)handlerPool.at(startPos + 1) == 0x34)
                            {
                                break;
                            }
                        }
                        break;
                    }

                    //发现停止指令
                    if(foundStop) {
                        //清空所有数据
                        handlerPool.clear();
                        workStatus = WorkEnd;
                        //计算出初始活度乘以几何效率，这里几何效率是近端探测器几何效率
                        double At_omiga = coincidenceAnalyzer->getInintialActive(detectorParameter, time_SetEnWindow);
                        activeOmigaToYield(At_omiga);
                        emit sigMeasureStop();
                        break;
                    }
                }
                else{ //查找有效数据包
                    // 先尝试寻找完整数据包（数据包、指令包两类）
                    quint32 size = handlerPool.size();

                    if(startPos >= size) break;

                    if (size >= minPkgSize){
                        // DataHead 寻找包头 00 00 aa b3 
                        for (startPos; startPos < handlerPool.size() - 3; startPos++)
                        {
                            if ((quint8)handlerPool.at(startPos) == 0x00 && (quint8)handlerPool.at(startPos + 1) == 0x00
                                && (quint8)handlerPool.at(startPos + 2) == 0xaa && (quint8)handlerPool.at(startPos + 3) == 0xb3)
                            {
                                HeadIndex = startPos;
                                break;
                            }
                        }

                        // DataTail 寻找包尾 00 00 cc d3
                        // 有包头的情况下，才找包尾
                        if(HeadIndex >= 0){
                            int posTail = HeadIndex + minPkgSize - 4;
                            if(posTail >= size -3) break;

                            // 直接尝试找到包尾
                            if ((quint8)handlerPool.at(posTail) == 0x00 && (quint8)handlerPool.at(posTail + 1) == 0x00
                                && (quint8)handlerPool.at(posTail + 2) == 0xcc && (quint8)handlerPool.at(posTail + 3) == 0xd3)
                            {
                                TailIndex = posTail;
                                startPos = TailIndex + 4;
                                isNual = true;
                            }
                        }
                    }
                    else{
                        break;
                    }

                    if ((HeadIndex == -1) || (TailIndex == -1)){ // 没找到包头包尾,并且包长度满足一包数据的最小值
                        continue;
                    }
                
                    //找到一个有效数据包
                    if (isNual){
                        //复制有效数据
                        // QByteArray validFrame(handlerPool.constData()+HeadIndex, minPkgSize);

                        //使用指针重构（最高效但需谨慎）, 不移动数据，时间复杂度 O(1)
                        //handlerPool = QByteArray(handlerPool.constData() + minPkgSize, handlerPool.size() - minPkgSize);
                        // char* data = handlerPool.data();
                        // memmove(data, data + minPkgSize, handlerPool.size() - minPkgSize);
                        // handlerPool.resize(handlerPool.size() - minPkgSize);

                        //处理数据
                        const unsigned char *ptrOffset = (const unsigned char *)handlerPool.constData();
                        ptrOffset += HeadIndex;

                        //通道号(4字节)
                        ptrOffset += 4; //包头4字节
                        quint32 channel = static_cast<quint32>(ptrOffset[0]) << 24 |
                                        static_cast<quint32>(ptrOffset[1]) << 16 |
                                        static_cast<quint32>(ptrOffset[2]) << 8 |
                                        static_cast<quint32>(ptrOffset[3]);

                        //通道值转换
                        channel = (channel == 0xFFF1) ? 0 : 1;

                        //序号（4字节）
                        quint32 dataNum = static_cast<quint32>(ptrOffset[4]) << 24 |
                                        static_cast<quint32>(ptrOffset[5]) << 16 |
                                        static_cast<quint32>(ptrOffset[6]) << 8 |
                                        static_cast<quint32>(ptrOffset[7]);

                        ptrOffset += 8;
                        //粒子模式数据(PARTICLE_NUM_ONE_PAKAGE+1)*16byte,6字节:时间，2字节:死时间，2字节:幅度
                        int ref = 0;
                        quint64 firsttime_temp = 0;
                        quint64 lasttime_temp = 0;
                        std::vector<TimeEnergy> temp;
                        while (ref++ < PARTICLE_NUM_ONE_PAKAGE){
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

                            if(ref == 1) firsttime_temp = t;
                            if(t>0) lasttime_temp = t; //一直更新最后一个数值，单是要确保t不是空值

                            if (t != 0x00 && amplitude != 0x00)
                                temp.push_back(TimeEnergy(t, deathtime, amplitude));
                        }

                        //对丢包情况进行修正处理
                        //考虑到实际丢包总是在大计数率下，网口传输响应不过来，两个通道的不响应时间长度基本相同，这里直接以探测器1的丢包来修正。
                        if(channel == 0){
                            quint32 losspackageNum = dataNum - SequenceNumber - 1; //注意要减一才是丢失的包个数
                            if(losspackageNum > 0) {
                                firstTime = firsttime_temp;
                                quint64 delataT = firstTime - lastTime;//丢包的时间段长度
                                
                                //检测丢包跨度是否刚好跨过某一秒的前后，
                                //经过初步测试，丢包的时候从来没有连续丢失超过1s的数据。
                                unsigned int leftT = static_cast<unsigned int>(ceil(lastTime*1.0/1e9)); //向上取整
                                unsigned int rightT = static_cast<unsigned int>(ceil(firstTime*1.0/1e9));

                                //对时间跨度刚好为一秒，则修复
                                if( rightT - leftT == 1){
                                    unsigned long long t1 = 0LL, t2 = 0LL;
                                    t1 = static_cast<unsigned long long>(leftT)*1e9 - lastTime;
                                    t2 = firstTime - static_cast<unsigned long long>(leftT)*1e9;
                                    if(t1>0 && t2>0)
                                    {
                                        lossData[leftT] += t1; //注意计时从1开始，因为是向上取整
                                        lossData[rightT] += t2; //注意计时从1开始
                                        coincidenceAnalyzer->AddLossMap(leftT, t1);
                                        coincidenceAnalyzer->AddLossMap(rightT, t2);
                                    }
                                }
                                else if((rightT - leftT) == 0){
                                    // 记录丢失的时间长度
                                    lossData[leftT] += delataT;
                                }
                                else{//暂时不考虑丢包超过两秒时长的修复
                                    
                                }
                                
                            }

                            //记录数据帧序号
                            SequenceNumber = dataNum;
                            //记录数据帧最后时间
                            lastTime = lasttime_temp;
                        }

                        //数据分拣完毕
                        DetTimeEnergy detTimeEnergy;
                        detTimeEnergy.channel = channel;
                        detTimeEnergy.timeEnergy.swap(temp);
                        {
                            QMutexLocker locker(&mutexPlot);
                            currentSpectrumFrames.push_back(detTimeEnergy);
                        }
                    }
                }
            }
            handlerPool.remove(0, startPos);
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
                        if ((quint8)handlerPool.at(0) == 0x00 && (quint8)handlerPool.at(1) == 0x00 && (quint8)handlerPool.at(2) == 0xaa && (quint8)handlerPool.at(3) == 0xb1){
                            // 寻找包尾(正常情况包尾正确)
                            if ((quint8)handlerPool.at(minPkgSize-4) == 0x00 && (quint8)handlerPool.at(minPkgSize-3) == 0x00 && (quint8)handlerPool.at(minPkgSize-2) == 0xcc && (quint8)handlerPool.at(minPkgSize-1) == 0xd1){
                                handlerPool.remove(0, minPkgSize);
                                count++;
                                continue;
                            } else {
                                handlerPool.remove(0, 1);//正常情况这个地方不应该进来
                            }     
                        } else {                            
                            //正常情况这个地方不应该进来

                            //先判断一下是否是停止指令插入进来了
                            QByteArray cmd = handlerPool.left(cmdStopTrigger.size());
                            if (cmd.compare(cmdStopTrigger) == 0){
                                qDebug()<<"Recv2 HEX: "<<cmdStopTrigger.toHex(' ');

                                QMutexLocker locker(&mutexFile);
                                if (nullptr != pfSaveNet){
                                    pfSaveNet->close();
                                    delete pfSaveNet;
                                    pfSaveNet = nullptr;
                                }

                                //测量停止是否需要清空所有数据
                                handlerPool.clear();
                                workStatus = WorkEnd;

                                slotStopManualMeasure(); //再次发送停止指令
                                emit sigMeasureStopWave();
                                break;
                            } else {
                                handlerPool.remove(0, 1);
                            }
                        }
                    } else if (handlerPool.size() == 12){
                        if (handlerPool.compare(cmdStopTrigger) == 0){                            
                            qDebug()<<"Recv HEX: "<<cmdStopTrigger.toHex(' ');

                            {
                                QMutexLocker locker(&mutexFile);
                                if (nullptr != pfSaveNet){
                                    pfSaveNet->close();
                                    delete pfSaveNet;
                                    pfSaveNet = nullptr;
                                }
                            }

                            //测量停止是否需要清空所有数据
                            handlerPool.clear();
                            workStatus = WorkEnd;
                            emit sigMeasureStopWave();
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

                //2、处理缓存区数据
                if (swapFrames.size() > 0){
                    for (size_t i=0; i<swapFrames.size(); ++i){
                        DetTimeEnergy detTimeEnergy = swapFrames.at(i);

                        // 根据步长，将数据添加到当前处理缓存
                        quint8 channel = detTimeEnergy.channel;
                        if (channel != 0x00 && channel != 0x01){
                            qDebug() << "error";
                        }

                        for (size_t i=0; i<detTimeEnergy.timeEnergy.size(); ++i){
                            TimeEnergy timeEnergy = detTimeEnergy.timeEnergy.at(i);

                            if (channel == 0x00){
                                data1_2.push_back(timeEnergy);
                            } else {
                                data2_2.push_back(timeEnergy);
                            }
                        }
                    }

                    if (data1_2.size() > 0 || data2_2.size() > 0 ){
                        QDateTime startTime = QDateTime::currentDateTime();
                        
                        if (detectorParameter.measureModel == mmAuto){//自动测量，需要获取能宽
                            double time1 = 0.0, time2 = 0.0;
                            if(data1_2.size() > 0) time1 = data1_2.begin()->time * 1.0 / 1e9;
                            if(data2_2.size() > 0) time2 = data2_2.begin()->time * 1.0 / 1e9;
                            //在冷却时长之后的数据才进行处理
                            if(time1 >= detectorParameter.coolingTime || time2 >= detectorParameter.coolingTime)
                            {
                                saveParticleInfo(data1_2, data2_2);
                                unsigned short uEnWindow[4];
                                uEnWindow[0] = (unsigned short)((EnWindow[0] - g_enCalibration[1]) / g_enCalibration[0]);
                                uEnWindow[1] = (unsigned short)((EnWindow[1] - g_enCalibration[1]) / g_enCalibration[0]);
                                uEnWindow[2] = (unsigned short)((EnWindow[2] - g_enCalibration[1]) / g_enCalibration[0]);
                                uEnWindow[3] = (unsigned short)((EnWindow[3] - g_enCalibration[1]) / g_enCalibration[0]);
                                coincidenceAnalyzer->calculate(data1_2, data2_2, uEnWindow, \
                                    detectorParameter.timeWidth, detectorParameter.delayTime, true, true);
                            }
                        }
                        else if(detectorParameter.measureModel == mmManual)
                        {
                            //在选定能窗前不进行符合数据处理，也就是不给出计数曲线，只有能谱数据。
                            if (this->autoChangeEnWindow)
                            {
                                //这里特别需要注意的是，由于此刻coincidenceAnalyzer中还有未处理完的数据unusedData1、unusedData2.
                                //这部分数据漏存，会导致在线测量给出的计数曲线与离线分析的计数曲线起始两个点不一样
                                if (startSaveValidData && coincidenceAnalyzer->GetPointPerSeconds().size()>0){
                                    // 则记录下计数曲线的起始时刻
                                    this->time_SetEnWindow = coincidenceAnalyzer->GetPointPerSeconds().back().time;
                                    // 记录到配置文件
                                    QString configResultFile = getFilenamePrefix() + "_配置.txt";
                                    {
                                        QFile::OpenMode ioFlags = QIODevice::Truncate;
                                        if (QFileInfo::exists(configResultFile))
                                            ioFlags = QIODevice::Append;
                                        QFile file(configResultFile);
                                        if (file.open(QIODevice::ReadWrite | QIODevice::Text | ioFlags)) {
                                            QTextStream out(&file);
                                            out << tr("测量开始时间(冷却时间+FPGA时钟)=")<< this->time_SetEnWindow <<Qt::endl;
                                            file.flush();
                                            file.close();
                                        }
                                    }
                                    saveParticleInfo(coincidenceAnalyzer->GetUnusedData1(), coincidenceAnalyzer->GetUnusedData2());
                                    startSaveValidData = false;
                                }
                                saveParticleInfo(data1_2, data2_2);
                                unsigned short uEnWindow[4];
                                uEnWindow[0] = (unsigned short)((EnWindow[0] - g_enCalibration[1]) / g_enCalibration[0]);
                                uEnWindow[1] = (unsigned short)((EnWindow[1] - g_enCalibration[1]) / g_enCalibration[0]);
                                uEnWindow[2] = (unsigned short)((EnWindow[2] - g_enCalibration[1]) / g_enCalibration[0]);
                                uEnWindow[3] = (unsigned short)((EnWindow[3] - g_enCalibration[1]) / g_enCalibration[0]);
                                coincidenceAnalyzer->calculate(data1_2, data2_2, uEnWindow, \
                                    detectorParameter.timeWidth, detectorParameter.delayTime, true, true);
                            }
                            else
                            {
                                //只计算能谱数据，不进行符合计数
                                // qDebug().noquote()<<"Into Size1 = "<<data1_2.size()<<", Size2 = "<<data2_2.size();
                                unsigned short uEnWindow[4];
                                uEnWindow[0] = (unsigned short)((EnWindow[0] - g_enCalibration[1]) / g_enCalibration[0]);
                                uEnWindow[1] = (unsigned short)((EnWindow[1] - g_enCalibration[1]) / g_enCalibration[0]);
                                uEnWindow[2] = (unsigned short)((EnWindow[2] - g_enCalibration[1]) / g_enCalibration[0]);
                                uEnWindow[3] = (unsigned short)((EnWindow[3] - g_enCalibration[1]) / g_enCalibration[0]);
                                coincidenceAnalyzer->calculate(data1_2, data2_2, uEnWindow, \
                                    detectorParameter.timeWidth, detectorParameter.delayTime, false, false);
                                // qDebug().noquote()<<"Out";
                            }
                        }
#ifdef QT_DEBUG
                        double time0 = 0.0;
                        if(data1_2.size() > 0) time0 = data1_2[0].time/1e9;
                        else time0 = data2_2[0].time/1e9;
                        qDebug()<< "coincidenceAnalyzer->calculate time=" << startTime.msecsTo(QDateTime::currentDateTime())
                                 <<"ms, time0="<<time0 \
                        << "s, data1.count=" << data1_2.size() \
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

/**
 * @description: 保存符合测量的粒子信息，触发时刻、死时间、能量。
 * @return {*}
 */
void CommandHelper::saveParticleInfo(const vector<TimeEnergy>& data1_2, const vector<TimeEnergy>& data2_2)
{
    //需要注意的是，对于手动测量和自动测量，计数曲线的时间轴处理有差异。
    //(1)手动测量
    //在冷却时间之前，放射源已经冷却了一段时间，这个冷却时间是界面的输入值，
    //然后用户在测量一定时间之后选取能窗进行高斯拟合，给出计数曲线的能窗，此后开始产生计数曲线。这段时间称为能窗选取时间。
    //因此用户在计数曲线上看到的起始时间应该是两个轴加起来的总时间。
    //计数起始时刻 = 冷却时间 + 能窗选取时刻FPGA内部时钟）
    //(2)自动测量
    //在硬件触发之后，FPGA开始计数并且采集数据上传，界面在冷却时间之前都不保存这段数据，直接解包之后丢弃。
    //判断时间在冷却时间之后的数据，则开始保存。
    //计数起始时刻 = 冷却时间
    if (pfSaveVaildData != nullptr){
        //探测器1
        quint32 size1 = data1_2.size();
        unsigned char channel = 0;
        if(size1>0)
        {
            pfSaveVaildData->write((char*)&size1, sizeof(quint32));
            //探测器编号0/1:1字节
            pfSaveVaildData->write((char*)&channel, sizeof(unsigned char));
            //数据对:12字节对
            pfSaveVaildData->write((char*)data1_2.data(), sizeof(TimeEnergy)*size1);
            pfSaveVaildData->flush();
        }

        //探测器2
        quint32 size2 = data2_2.size();
        channel = 1;
        if(size2>0)
        {
            pfSaveVaildData->write((char*)&size2, sizeof(quint32));
            //探测器编号0/1:1字节
            pfSaveVaildData->write((char*)&channel, sizeof(unsigned char));
            //数据对:12字节对
            pfSaveVaildData->write((char*)data2_2.data(), sizeof(TimeEnergy)*size2);
            pfSaveVaildData->flush();
        }
    }
}

void CommandHelper::updateStepTime(int _stepT)
{
    QMutexLocker locker(&mutexReset);
    this->stepT = _stepT;
}

void CommandHelper::updateParamter(int _stepT, double _EnWin[4], bool _autoEnWindow)
{
    QMutexLocker locker(&mutexReset);

    this->stepT = _stepT;
    this->EnWindow[0] = _EnWin[0];
    this->EnWindow[1] = _EnWin[1];
    this->EnWindow[2] = _EnWin[2];
    this->EnWindow[3] = _EnWin[3];
    
    this->autoChangeEnWindow = _autoEnWindow;
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
        if (this->detectorParameter.transferModel == 0x05){
        }
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


bool CommandHelper::checkAndClearQByteArray(QByteArray &data) {
    if (data.capacity() > MAX_BYTEARRAY_SIZE) {
        qCritical().noquote()<< "探测器计数率超出当前仪器计数率设计的上限，数据会发生丢失，导致计数率失真。\n"
                   <<"可能原因: 1)、放射性活度过高; 2)波形触发阈值过低导致。";
        data.clear();
        data.squeeze(); // 释放内存
        return true;
    }
    return false;
}
