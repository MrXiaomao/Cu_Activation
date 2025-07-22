/*
 * @Author: MrPan
 * @Date: 2025-04-20 09:21:28
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-07-22 23:02:02
 * @Description: 离线数据分析
 */
#include "offlinedataanalysiswidget.h"
#include "ui_offlinedataanalysiswidget.h"
#include "plotwidget.h"
#include "splashwidget.h"

#include <QButtonGroup>
#include <QFileDialog>
#include <QAction>
#include <QToolButton>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <math.h>

OfflineDataAnalysisWidget::OfflineDataAnalysisWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OfflineDataAnalysisWidget)
{
    ui->setupUi(this);
    initCustomPlot();

    ui->label_tag->installEventFilter(this);
    ui->label_tag_3->installEventFilter(this);
    ui->label_measure_tag->installEventFilter(this);
    
    ui->label_measure_tag->setBuddy(ui->widget_measure_tag);
    ui->label_tag->setBuddy(ui->widget_tag);
    ui->label_tag_3->setBuddy(ui->widget_tag_3);

    QAction *action = ui->lineEdit_savepath->addAction(QIcon(":/resource/open.png"), QLineEdit::TrailingPosition);
    QToolButton* button = qobject_cast<QToolButton*>(action->associatedWidgets().last());
    button->setCursor(QCursor(Qt::PointingHandCursor));
    connect(button, &QToolButton::pressed, this, [=](){
        QString cacheDir = QFileDialog::getExistingDirectory(this);
        if (!cacheDir.isEmpty()){
            ui->lineEdit_savepath->setText(cacheDir);
        }
    });

    //ui->tableWidget1->resizeColumnsToContents();
    ui->tableWidget1->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for(int i=0; i<3; i++){
        for(int j=3; j<6; j++){
            ui->tableWidget1->item(j, i)->setText(QString::number(0));
        }
    }

    connect(this, &OfflineDataAnalysisWidget::sigNewPlot, this, [=](SingleSpectrum r1, vector<CoincidenceResult> r3){
        bool pause_plot = this->property("pause_plot").toBool();
        if (!pause_plot){
            PlotWidget* plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
            // QDateTime now = QDateTime::currentDateTime();
            plotWidget->slotUpdatePlotDatas(r1, r3);
            // qDebug()<< "plot time=" << now.msecsTo(QDateTime::currentDateTime())\
            //              << ", r3.size()=" << r3.size();
        }
    }, Qt::DirectConnection/*防止堵塞*/);

    QButtonGroup *grp = new QButtonGroup(this);
    grp->addButton(ui->radioButton_ref, 0);
    grp->addButton(ui->radioButton_spectrum, 1);
    connect(grp, QOverload<int>::of(&QButtonGroup::idClicked), this, [=](int index){
        PlotWidget* plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
        plotWidget->switchToCountMode(index == 0);
    });
    emit grp->idClicked(1);//默认能谱模式

    coincidenceAnalyzer = new CoincidenceAnalyzer();
    coincidenceAnalyzer->set_callback([=](SingleSpectrum r1, vector<CoincidenceResult> r3) {
        this->updateCoincidenceData(r1, r3);
    });

    connect(this, SIGNAL(sigStart()), this, SLOT(slotStart()));
    connect(this, SIGNAL(sigEnd(bool)), this, SLOT(slotEnd(bool)), Qt::QueuedConnection);

    // 刷新测量结果
    connect(this, &OfflineDataAnalysisWidget::sigUpdate_Active_yield, this, [=](double active, double yield){
        ui->lineEdit_realativeA0->setText(QString::number(active, 'g', 9));
        ui->lineEdit_neutronYield->setText(QString::number(yield, 'E', 6));
    }, Qt::QueuedConnection/*防止堵塞*/);

    connect(SplashWidget::instance(), &SplashWidget::sigCancel, this, [=](){
        reAnalyzer = true;
    });

    connect(ui->spinBox_1_leftE, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateEnWindow()));
    connect(ui->spinBox_1_rightE, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateEnWindow()));
    connect(ui->spinBox_2_leftE, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateEnWindow()));
    connect(ui->spinBox_2_rightE, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateEnWindow()));
    slotUpdateEnWindow();
}

void OfflineDataAnalysisWidget::slotUpdateEnWindow()
{
    unsigned short EnWin[4] = {(unsigned short)ui->spinBox_1_leftE->value(), (unsigned short)ui->spinBox_1_rightE->value(),
                               (unsigned short)ui->spinBox_2_leftE->value(), (unsigned short)ui->spinBox_2_rightE->value()};

    PlotWidget* plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
    plotWidget->slotUpdateEnWindow(EnWin);
}

OfflineDataAnalysisWidget::~OfflineDataAnalysisWidget()
{
    delete coincidenceAnalyzer;
    coincidenceAnalyzer = nullptr;

    delete ui;
}

#include <QMouseEvent>
bool OfflineDataAnalysisWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != this){
        if (event->type() == QEvent::MouseButtonPress){
            if (watched->inherits("QLabel")){
                QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
                if (e->button() == Qt::LeftButton) {
                    QLabel* label = qobject_cast<QLabel*>(watched);
                    bool toggled = false;
                    if (label->property("toggled").isValid())
                        toggled = label->property("toggled").toBool();

                    toggled = !toggled;
                    label->setProperty("toggled", toggled);
                    if (toggled){
                        label->setPixmap(QPixmap(":/resource/arrow-drop-up-x16.png"));
                        QWidget * w = label->buddy();
                        w->hide();
                    } else {
                        label->setPixmap(QPixmap(":/resource/arrow-drop-down-x16.png"));
                        QWidget * w = label->buddy();
                        w->show();
                    }
                }
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void OfflineDataAnalysisWidget::initCustomPlot()
{
    {
        PlotWidget *customPlotWidget = new PlotWidget(this);
        customPlotWidget->setObjectName("offline-PlotWidget");
        customPlotWidget->initCustomPlot();
        connect(customPlotWidget, &PlotWidget::sigUpdateEnWindow, this, [=](QString name, unsigned int leftEn, unsigned int rightEn){
            if (name == "Energy-Det1"){
                ui->spinBox_1_leftE->setValue(leftEn);
                ui->spinBox_1_rightE->setValue(rightEn);
            } else{
                ui->spinBox_2_leftE->setValue(leftEn);
                ui->spinBox_2_rightE->setValue(rightEn);
            }

        });

        connect(customPlotWidget, &PlotWidget::sigAreaSelected, this, [=](){
            //勾选高斯拟合
            ui->checkBox_gauss->setChecked(true);
        });

        ui->widget_plot->layout()->addWidget(customPlotWidget);
    }
}

//#include <iostream>
#include "qlitethread.h"
#include <QFileInfo>
#include <QTextStream>
#include <QTimer>
bool OfflineDataAnalysisWidget::LoadMeasureParameter(QString filePath)
{
    //重新初始化
    for(int i=0; i<3; i++){
        for(int j=3; j<6; j++){
            ui->tableWidget1->item(j, i)->setText(QString::number(0));
        }
    }

    ui->textBrowser_filepath->setText(filePath);
    validDataFileName = filePath;

    //先对旧版本配置文件命名的兼容 xxx_Net.dat xxx_Net.dat.配置
    //新版 xxx_Net.dat xxx_配置.txt
    //正则表达式
    QRegularExpression re_valid("^(.*?)_valid\\.dat$");
    QRegularExpression re_Net("^(.*?)_Net\\.dat$");
    QRegularExpressionMatch match1 = re_valid.match(filePath);
    QRegularExpressionMatch match2 = re_Net.match(filePath);
    QString prefix;
    if (match1.hasMatch()) {
        prefix = match1.captured(1);
    }
    else if(match2.hasMatch())
    {
        prefix = match2.captured(1);
    }
    else
    {
        return false;
    }

    QString configFile = prefix + "_配置.txt";
    if (!QFileInfo::exists(configFile)){
        //若文件不存在，则考虑是否为软件旧版本的命名风格
        configFile = filePath + ".配置";
        if (!QFileInfo::exists(configFile)){
            //若还是不存在该文件，则说明缺失测量配置文件
            return false;
        }
    }

    //只有选取到有效文件后才允许使用
    ui->pushButton_start->setEnabled(true);

    QFile file(configFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMap<QString, QString> configMap;
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue; // 跳过空行和注释行

            int separatorIndex = line.indexOf('=');
            if (separatorIndex != -1) {
                QString key = line.left(separatorIndex).trimmed();
                QString value = line.mid(separatorIndex + 1).trimmed();

                // 处理带单位的键和不带单位的键
                if(key.endsWith("(ns)") ) {
                    QString baseKey = key.left(key.length() - 4); // 去掉"(ns)"
                    if(!configMap.contains(baseKey)) { // 如果基础键不存在，则添加
                        configMap[baseKey] = value;
                    }
                }
                else if(key.endsWith("(s)"))
                {
                    QString baseKey = key.left(key.length() - 3); // 去掉"(s)"
                    if(!configMap.contains(baseKey)) { // 如果基础键不存在，则添加
                        configMap[baseKey] = value;
                    }
                }
                configMap[key] = value; // 保留原始键值对
            }
        }
        file.close();

        if(configMap.value("测量模式") == "手动") {
            detParameter.measureModel = mmManual;
            ui->spinBox_coolingTime->setEnabled(true); //手动模式允许修改冷却时长
        }
        else if(configMap.value("测量模式") == "自动") {
            detParameter.measureModel = mmAuto;
            ui->spinBox_coolingTime->setEnabled(false);//自动模式不允许修改冷却时长
        }
        detParameter.coolingTime = configMap.value("冷却时长").toInt();
        detParameter.timeWidth = configMap.value("符合分辨时间").toInt();
        
        QString rangeStr = configMap.value("量程选取");
        if(rangeStr == "小量程") detParameter.measureRange = 1;
        if(rangeStr == "中量程") detParameter.measureRange = 2;
        if(rangeStr == "大量程") detParameter.measureRange = 3;

        this->startTime_absolute = static_cast<unsigned int>(configMap.value("测量开始时间(冷却时间+FPGA时钟)").toInt());
        if(detParameter.measureModel == mmManual) startFPGA_time = this->startTime_absolute - detParameter.coolingTime;
        else startFPGA_time = detParameter.coolingTime + 1;

        ui->lineEdit_measuremodel->setText(configMap.value("测量模式"));
        ui->spinBox_step->setValue(configMap.value("时间步长").toInt());
        ui->spinBox_timeWidth->setValue(configMap.value("符合分辨时间").toInt());
        ui->spinBox_coolingTime->setValue(configMap.value("冷却时长").toInt());
        ui->lineEdit_range->setText(rangeStr);

        int minTime = 0;
        int maxTime = 0;
        // if(detParameter.measureModel == mmManual) {
        //     minTime = startTime_absolute + 1;
        //     maxTime = configMap.value("测量时长").toInt() + detParameter.coolingTime;
        // }else{
        minTime = startTime_absolute + 1;
        maxTime = configMap.value("测量时长").toInt() + detParameter.coolingTime;
        // }
        //读取配置参数的时候，默认数据分析参数为全部时间段测量数据。
        ui->spinBox_start->setRange(0, INT_MAX); // 先设置足够大的范围
        ui->spinBox_end->setRange(0, INT_MAX); // 先设置足够大的范围
        // ui->spinBox_start->setValue(minTime+2); //丢弃前两个点的数据，因为前两个点数据经常不完整
        ui->spinBox_start->setValue(minTime); //丢弃前两个点的数据，因为前两个点数据经常不完整
        ui->spinBox_end->setValue(maxTime);

        ui->spinBox_1_leftE->setValue(configMap.value("Det1符合能窗左").toInt());
        ui->spinBox_1_rightE->setValue(configMap.value("Det1符合能窗右").toInt());
        ui->spinBox_2_leftE->setValue(configMap.value("Det2符合能窗左").toInt());
        ui->spinBox_2_rightE->setValue(configMap.value("Det2符合能窗右").toInt());
    }
    return true;
}

#include <QMessageBox>
void OfflineDataAnalysisWidget::slotStart()
{
    qInfo().noquote()<<"开始解析历史数据";
    if (!analyzerFinished){
        QMessageBox::information(this, tr("提示"), tr("当前解析还未完成，请稍后重试！"));
        reAnalyzer = true;
        return;
    }

    firstPopup = true;
    reAnalyzer = false;
    isUpdateYield = false;
    detectNum = 0;
    lossData.clear();
    activationResult.clear();
    loadConfig();

    memset(&totalSingleSpectrum, 0, sizeof(totalSingleSpectrum));

    //读取历史数据重新进行运算
    PlotWidget* plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
    plotWidget->switchToMoveMode();
    plotWidget->slotStart();

    unsigned short EnWindow[4] = {(unsigned short)ui->spinBox_1_leftE->value(), (unsigned short)ui->spinBox_1_rightE->value(),
                               (unsigned short)ui->spinBox_2_leftE->value(), (unsigned short)ui->spinBox_2_rightE->value()};
    int delayTime = ui->spinBox_delayTime->value();//符合延迟时间,ns
    int timeWidth = ui->spinBox_timeWidth->value();//时间窗宽度，单位ns(符合分辨时间)
    detParameter.timeWidth = timeWidth;
    // if(detParameter.measureModel == mmManual) 
    detParameter.coolingTime = ui->spinBox_coolingTime->value(); //更新冷却时间，只有手动模式可以更新

    QLiteThread *calThread = new QLiteThread();
    calThread->setObjectName("calThread");
    qDebug() << "创建离线数据处理线程，id=" << calThread->thread()->currentThreadId();
    connect(calThread, &QThread::destroyed, this, [=]{
        qDebug() << "离线数据处理线程退出，id=" << calThread->thread()->currentThreadId();
    });
    calThread->setWorkThreadProc([=](){
        coincidenceAnalyzer->initialize();

        if(detParameter.measureModel == mmManual){
            coincidenceAnalyzer->setCoolingTime_Manual(detParameter.coolingTime);
        }
        //注意：手动模式、自动模式都调用这个语句，用来设置前面一段没有保存数据的时长。手动测量在“确认能窗”前不保存数据。
        coincidenceAnalyzer->setCoolingTime_Auto(startFPGA_time);

        QByteArray aDatas = validDataFileName.toLocal8Bit();
        vector<TimeEnergy> data1_2, data2_2;
// #ifdef QT_NO_DEBUG
        SysUtils::realQuickAnalyzeTimeEnergy((const char*)aDatas.data(), [&](DetTimeEnergy detTimeEnergy, \
            unsigned long long progress/*文件进度*/, unsigned long long filesize/*文件大小*/, bool eof, bool *interrupted){
// #else
        // SysUtils::realAnalyzeTimeEnergy((const char*)aDatas.data(), [&](DetTimeEnergy detTimeEnergy,
            // unsigned long long progress/*文件进度*/, unsigned long long filesize/*文件大小*/, bool eof, bool *interrupted){
// #endif
            if (firstPopup && !eof){
                QTimer::singleShot(1, this, [=](){
                    SplashWidget::instance()->setInfo(tr("文件正在解析中，请等待..."), true, true);
                    SplashWidget::instance()->exec();
                });

                firstPopup = false;
            }

            emit SplashWidget::instance()->sigUpdataProgress(progress, filesize);

            if (eof){
                if (interrupted)
                    emit sigEnd(true);
                else{
                    emit sigEnd(false);
                }

                return;
            }

            quint8 channel = detTimeEnergy.channel;

            if (channel == 0x00){
                detectNum += detTimeEnergy.timeEnergy.size();
                data1_2 = detTimeEnergy.timeEnergy;
            } else if(channel == 0x01){
                detectNum += detTimeEnergy.timeEnergy.size();
                data2_2 = detTimeEnergy.timeEnergy;
            }
            else if(channel == 0x02){ //请注意，这里对于0x02通道其实放的是FPGA修正时刻，具体参见commandhelper中信号sigMeasureStop的关联函数部分
                //解析还原出数据
                std::vector<TimeEnergy> t_En = detTimeEnergy.timeEnergy;
                for (const TimeEnergy& te : t_En) {
                    // 注意数据存储结构的对齐问题，
                    unsigned long long deltaTime_ns = te.time;
                    unsigned int time_seconds = static_cast<quint32>(te.dietime) << 16 | 
                                        static_cast<quint32>(te.energy);
                    lossData[time_seconds] = deltaTime_ns;
                }
            }
            
            //记录FPGA内的最大时刻，作为符合测量的时间区间右端点。
            if (data1_2.size() > 0 || data2_2.size() > 0 ){
                coincidenceAnalyzer->calculate(data1_2, data2_2, (unsigned short*)EnWindow, timeWidth, delayTime, true, true);
                data1_2.clear();
                data2_2.clear();
            }

            *interrupted = reAnalyzer;
        });
    });
    calThread->start();
}

void OfflineDataAnalysisWidget::updateCoincidenceData(SingleSpectrum r1, vector<CoincidenceResult> r3)
{
    size_t count = r3.size();
    int _stepT = ui->spinBox_step->value();
    if (count <= 0)
        return;

    //保存信息
    {
        for (int i=0; i<MULTI_CHANNEL; ++i){
            totalSingleSpectrum.spectrum[0][i] += r1.spectrum[0][i];
            totalSingleSpectrum.spectrum[1][i] += r1.spectrum[1][i];
        }

        //临时测试，保留该代码，后续调参数需要使用。优化maxTime_updateYield、deltaTime_updateYield的选取
        //在线测量中，对前maxTime_updateYield的时间内数据，定时给出中子产额。
        //为了评估该数值取多少合适，这里对其进行考察，也可以借助matlab代码来优化该数值的选取
        if(isUpdateYield)
        {
            if(count >0 && count <=maxTime_updateYield && count%deltaTime_updateYield==0){
                //选取已测的全部数据点给出中子产额
                int start_time = r3.at(0).time - 1;
                int end_time = r3.back().time;
                //选取最近的一段时间给出中子产额
                // int start_time = r3.back().time - deltaTime_updateYield;
                // int end_time = r3.back().time;
                double At_omiga = coincidenceAnalyzer->getInintialActive(detParameter, start_time, end_time);
                activeOmigaToYield(At_omiga);
            }
        }

        // 先不着急刷新图像和更新计数率，等数据全部处理完了再一次性刷新，避免耗时太久
        return;
    }

    //时间步长，求均值
    if (_stepT > 1){
        if (count>1 && (count % _stepT == 0)){
            vector<CoincidenceResult> rr3;

            size_t posI = 0;
            for (size_t i=0; i < count/_stepT; i++){
                CoincidenceResult v;
                for (int j=0; j<_stepT; ++j){
                    posI = i*_stepT + j;
                    v.CountRate1 += r3[posI].CountRate1;
                    v.CountRate2 += r3[posI].CountRate2;
                    v.ConCount_single += r3[posI].ConCount_single;
                    v.ConCount_multiple += r3[posI].ConCount_multiple;
                }

                //给出平均计数率cps,注意，这里是整除，当计数率小于1cps时会变成零。
                v.time = r3[posI].time;
                v.CountRate1 /= _stepT;
                v.CountRate2 /= _stepT;
                v.ConCount_single /= _stepT;
                v.ConCount_multiple /= _stepT;
                rr3.push_back(v);
            }

            emit sigNewPlot(r1, rr3);
        }
    } else{
        // vector<CoincidenceResult> rr3;
        // for (size_t i=0; i < count; i++){
        //     rr3.push_back(r3[i]);
        // }
        emit sigNewPlot(r1, r3);
    }
}

// 将相对活度转化为中子产额
void OfflineDataAnalysisWidget::activeOmigaToYield(double active)
{
    //本次测量量程
    int measureRange = detParameter.measureRange-1; //这里涉及到界面下拉框从0开始计数，而detParameter中从1开始计数。
    //读取配置文件，给出该量程下的刻度系数
    double cali_factor = 0.0;
    QFile file(QApplication::applicationDirPath() + "/config/user.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();
        
        QJsonObject jsonCalibration, jsonYield;
        if (jsonObj.contains("YieldCalibration")){
            jsonCalibration = jsonObj["YieldCalibration"].toObject();
            
            QString key = QString("Range%1").arg(measureRange);
            QJsonArray rangeArray;
            if (jsonCalibration.contains(key)){
                rangeArray = jsonCalibration[key].toArray();
                QJsonObject rangeData = rangeArray[0].toObject();

                double yield = rangeData["Yield"].toDouble();
                double active0 = rangeData["active0"].toDouble();
                cali_factor = yield / active0;
            }
        }
    }
    else{
        qCritical().noquote()<<"未找到拟合参数，请检查仪器是否进行了刻度,请对仪器刻度后重新计算（采用‘数据查看和分析’子界面分析）";
    }

    double result = active * cali_factor;
    
    //保存结果
    yieldResult oneResult;
    oneResult.time = coincidenceAnalyzer->GetCoinResult().back().time;
    oneResult.ActiveOmiga = active;
    oneResult.Yield = result;
    activationResult.push_back(oneResult);

    sigUpdate_Active_yield(active, result);
}

/**
 * @description: 分析数据，更改数据分析参数后，重新读取文件，分析数据。
 * @return {*}
 */
void OfflineDataAnalysisWidget::on_pushButton_start_clicked()
{
    emit sigStart();
}

void OfflineDataAnalysisWidget::slotEnd(bool interrupted)
{
    //结束线程,释放指针，避免野指针。
    QLiteThread *thread = this->findChild<QLiteThread*>("calThread");
    if (thread) {
        qDebug() << "Found thread:" << thread->objectName();
        thread->quit();
        thread->wait();
        delete thread;
    }

    if (interrupted){
        QTimer::singleShot(1, this, [=](){
            SplashWidget::instance()->hide();
            QMessageBox::information(this, tr("提示"), tr("文件解析意外被终止！"));
            qInfo().noquote()<<"历史数据解析意外被终止!";
        });
    }
    else
    {
        SplashWidget::instance()->setInfo(tr("开始数据分析，请等待..."));
        
        //进行FPGA丢包的修正
        coincidenceAnalyzer->doFPGA_lossDATA_correction(lossData);
        
        // 读取活化测量的数据时刻区间[起始时间，结束时间]
        coinResult = coincidenceAnalyzer->GetCoinResult();
        if(coinResult.size()==0){
            QTimer::singleShot(1, this, [=](){
                qCritical().noquote()<<"历史数据解析失败";
                SplashWidget::instance()->hide();
                QMessageBox::information(this, tr("提示"), tr("文件解析错误，文件中不存在有效测量数据！"));
            });
            return;
        }
        unsigned int startTime = coinResult.at(0).time;
        unsigned int endTime = coinResult.back().time;

        //检查界面的起始时刻和结束时刻不超范围，若超范围则调整到范围内。
        startTimeUI = static_cast<unsigned int>(ui->spinBox_start->value());
        endTimeUI = static_cast<unsigned int>(ui->spinBox_end->value());

        if(startTimeUI < startTime || startTimeUI > endTime){
            ui->spinBox_start->setValue(startTime);
            startTimeUI = startTime;
        }

        if(endTimeUI < startTime || endTimeUI > endTime){
            ui->spinBox_end->setValue(endTime);
            endTimeUI = endTime;
        }

        //更新控件输入范围
        ui->spinBox_start->setRange(startTime, endTime);  // 设置范围
        ui->spinBox_end->setRange(startTime, endTime);  // 设置范围

        analyse(detParameter, startTimeUI, endTimeUI);

        //在这里一次性更新图像和计数率，可以最大程度减小系统损耗时间
        {
            size_t count = coinResult.size();
            int _stepT = ui->spinBox_step->value();

            //时间步长，求均值
            if (_stepT > 1){
                // int validCount = count - startFPGA_time; //由于符合曲线的前面部分填充了零。
                int validCount = count; //后续修改后，计数曲线前面部分没有填0的部分了。
                if (validCount>1){
                    vector<CoincidenceResult> rr3;
                    size_t count = validCount/_stepT;
                    size_t posI = 0;
                    for (size_t i=0; i < count; i++){
                        CoincidenceResult v;
                        for (int j=0; j<_stepT; ++j){
                            posI = i*_stepT + j;
                            v.CountRate1 += coinResult[posI].CountRate1;
                            v.CountRate2 += coinResult[posI].CountRate2;
                            v.ConCount_single += coinResult[posI].ConCount_single;
                            v.ConCount_multiple += coinResult[posI].ConCount_multiple;
                        }

                        //给出平均计数率cps,注意，这里是整除，当计数率小于1cps时会变成零。
                        v.time = coinResult[posI].time;
                        v.CountRate1 /= _stepT;
                        v.CountRate2 /= _stepT;
                        v.ConCount_single /= _stepT;
                        v.ConCount_multiple /= _stepT;
                        rr3.push_back(v);
                    }

                    emit sigNewPlot(totalSingleSpectrum, rr3);
                }
            } else{
                // vector<CoincidenceResult> rr3;
                // for (size_t i=0; i < count; i++){
                //     rr3.push_back(coinResult[i]);
                // }

                emit sigNewPlot(totalSingleSpectrum, coinResult);
            }
        }

        QTimer::singleShot(1, this, [=](){
            qInfo().noquote()<<"历史数据解析顺利完成";
            SplashWidget::instance()->hide();
            QMessageBox::information(this, tr("提示"), tr("文件解析已顺利完成！"));
        });
    }
}

/**
 * @description: 在计算出符合计数曲线之后，进行进一步分析，对与选定的时间区间进行数据处理。给出探测器1计数、探测器2计数、符合计数的最小值、最大值、平均值。
 * 以及死时间修正后的数值，探测器1总计数、探测器2总计数、总符合计数。
 * 计算出初始相对活度、中子产额。
 * 参考自CoincidenceAnalyzer::getInintialActive,注意离线处理产生的符合数据包含了冷却时间的零计数。与在线处理有一定区别
 * @param {DetectorParameter} detPara
 * @param {int} start_time 这start_time指曲线上数据点的x值。比如计数点[t=1,count=220]~[t=30,count=180]这两个点区间。则startT=1,endT=30
                *****特别注意，这里的start_time和在线处理getInintialActive(detPara, int start_time, int time_end)
 *              中的start_time是不一样的，
 * @param {int} time_end
 * @return {*}
 */
void OfflineDataAnalysisWidget::analyse(DetectorParameter detPara, unsigned int start_time, unsigned int time_end)
{    
    //如果没有符合计算结果，那么后续计算不能进行
    if(coinResult.size() == 0) return ;

    // 根据量程，自动获取本量程下对应的中子产额刻度参数——Cu62与Cu64的初始β+活度分支比，本底全能峰位置的4*sigma下计数率，
    // 如果缺失参数则直接采用默认值。
    int measureRange = detPara.measureRange - 1;
    double ratioCu62 = 0.0, ratioCu64 = 1.0;
    double backRatesDet1 = 0.0, backRatesDet2 = 0.0;
    QFile file(QApplication::applicationDirPath() + "/config/user.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();
        
        QJsonObject jsonCalibration, jsonYield;
        if (jsonObj.contains("YieldCalibration")){
            jsonCalibration = jsonObj["YieldCalibration"].toObject();
            
            QString key = QString("Range%1").arg(measureRange);
            QJsonArray rangeArray;
            if (jsonCalibration.contains(key)){
                rangeArray = jsonCalibration[key].toArray();
                QJsonObject rangeData = rangeArray[0].toObject();
                
                double ratio1 = rangeData["branchingRatio_Cu62"].toDouble();
                double ratio2 = rangeData["branchingRatio_Cu64"].toDouble();
                backRatesDet1 = rangeData["backgroundRatesDet1"].toDouble();
                backRatesDet2 = rangeData["backgroundRatesDet2"].toDouble();
                //归一化
                ratioCu62 = ratio1 / (ratio1 + ratio2);
                ratioCu64 = ratio2 / (ratio1 + ratio2);
            }
            else{
                qCritical().noquote()<<"离线数据分析：未找到对应量程的拟合参数，请检查仪器是否进行了相应量程刻度,请对仪器刻度后重新计算";
            }
        }
    }
    else{
        qCritical().noquote()<<"离线数据分析：未找到拟合参数，请检查仪器是否进行了刻度,请对仪器刻度后重新计算";
    }

    size_t num = time_end - start_time + 1; //计数点个数。

    //符合时间窗,单位ns
    double timeWidth_tmp = detPara.timeWidth * 1.0;
    timeWidth_tmp = timeWidth_tmp / 1e9;

    if(time_end < start_time) return ; //不允许起始时间小于停止时间

    double N1 = 0;
    double N2 = 0;
    double Nc = 0;
    double deathTime_Ratio1 = 0.0;
    double deathTime_Ratio2 = 0.0;
    
    //修正前的极值
    int minCount[3] = {100000, 100000, 100000}; //这里初值不能是一个常规小的计数率。
    int maxCount[3] = {0, 0, 0};
    double minDeathRatio[3] = {1.0, 1.0, 1.0}; //注意这里初值不能为0.0
    double maxDeathRatio[3] = {0.0, 0.0, 0.0};
    
    //修正后的极值
    double minCount_correct[3] = {100000.0, 100000.0, 100000.0}; //初值不能是一个小的数值
    double maxCount_correct[3] = {0.0, 0.0, 0.0};
    for(auto coin:coinResult)
    {
        // 手动测量，在改变能窗之前的数据不处理，注意剔除。现在数据没有保存这一段数据
        if(reAnalyzer){
            return;
        }

        if(coin.time >= start_time && coin.time <= time_end) {
            int n1 = coin.CountRate1;
            int n2 = coin.CountRate2;
            int nc = coin.ConCount_single + coin.ConCount_multiple;
            double ratio1 = coin.DeathRatio1 * 0.01;
            double ratio2 = coin.DeathRatio2 * 0.01;
            double ratio3 = 1.0 - (1.0 - ratio1)*(1 - ratio2);

            minCount[0] = qMin(minCount[0], n1);
            minCount[1] = qMin(minCount[1], n2);
            minCount[2] = qMin(minCount[2], nc);
            
            maxCount[0] = qMax(maxCount[0], n1);
            maxCount[1] = qMax(maxCount[1], n2);
            maxCount[2] = qMax(maxCount[2], nc);
            
            minDeathRatio[0] = qMin(minDeathRatio[0], ratio1);
            minDeathRatio[1] = qMin(minDeathRatio[1], ratio2);
            minDeathRatio[2] = qMin(minDeathRatio[2], ratio3);
            
            maxDeathRatio[0] = qMax(maxDeathRatio[0], ratio1);
            maxDeathRatio[1] = qMax(maxDeathRatio[1], ratio2);
            maxDeathRatio[2] = qMax(maxDeathRatio[2], ratio3);

            // 死时间修正后各计数率
            double n10 = n1 / (1-ratio1);
            double n20 = n2 / (1-ratio2);
            double nc0 = nc / (1-ratio3);
            minCount_correct[0] = qMin(minCount_correct[0], n10);
            minCount_correct[1] = qMin(minCount_correct[1], n20);
            minCount_correct[2] = qMin(minCount_correct[2], nc0);

            maxCount_correct[0] = qMax(maxCount_correct[0], n10);
            maxCount_correct[1] = qMax(maxCount_correct[1], n20);
            maxCount_correct[2] = qMax(maxCount_correct[2], nc0);

            N1 += (n1 - backRatesDet1);
            N2 += (n2 - backRatesDet2);
            Nc += nc;
            deathTime_Ratio1 += ratio1;
            deathTime_Ratio2 += ratio2;
            // int N1_temp = int(N1);
            // int N2_temp = int(N2);
            // int Nc_temp = int(Nc);
            // std::cout<<coin.time<<","<<N1_temp<<","<<N2_temp<<","<<Nc_temp<<std::endl;
        }
    }
    
    //计算修正前平均值
    double countAverage[3] = {0.0, 0.0, 0.0};
    double deathRatioAverage[3] = {0.0, 0.0, 0.0};
    countAverage[0] = N1*1.0 / num;
    countAverage[1] = N2*1.0 / num;
    countAverage[2] = Nc*1.0 / num;
    deathRatioAverage[0] = deathTime_Ratio1 / num;
    deathRatioAverage[1] = deathTime_Ratio2 / num;
    deathRatioAverage[2] = 1.0 - (1.0 - deathRatioAverage[0])*(1 - deathRatioAverage[1]);
    
    // deathTime_Ratio1 = deathTime_Ratio1 / num;
    // deathTime_Ratio2 = deathTime_Ratio2 / num;
    
    //计算修正后平均值
    double countAverage_orrect[3] = {0.0, 0.0, 0.0};
    countAverage_orrect[0] = N1*1.0 / (1 - deathRatioAverage[0]) / num;
    countAverage_orrect[1] = N2*1.0 / (1 - deathRatioAverage[1]) / num;
    countAverage_orrect[2] = Nc*1.0 / (1 - deathRatioAverage[2]) / num;

    //f因子。暂且称为积分因子
    //这里不考虑Cu62的衰变分支，也就是测量必须时采用冷却时长远大于Cu62半衰期(9.67min = 580s)的数据。
    double halflife_Cu62 = 9.67*60; //单位s，Cu62的半衰期
    double halflife_Cu64 = 12.7*60*60; // 单位s,Cu64的半衰期
    double lamda62 = log(2) / halflife_Cu62;
    double lamda64 = log(2) / halflife_Cu64;
    //注意，起点位置要减一，时间积分的左端点
    double f = ratioCu62/lamda62*(exp(-lamda62*(start_time-1)) - exp(-lamda62*time_end)) + \
                ratioCu64/lamda64*(exp(-lamda64*(start_time-1)) - exp(-lamda64*time_end));
                
    //对符合计数进行真偶符合修正
    //注意timeWidth_tmp单位为ns，要换为时间s。
    double measureTime = (time_end - start_time + 1)*1.0;
    double Nco = (Nc*measureTime - 2*timeWidth_tmp*N1*N2)/(measureTime - timeWidth_tmp*(N1 + N2));

    //死时间修正
    double N10 = N1 / (1 - deathRatioAverage[0]);
    double N20 = N2 / (1 - deathRatioAverage[1]);
    double Nco0 = Nco / (1 - deathRatioAverage[0]) / (1 - deathRatioAverage[1]);
    //计算出活度乘以探测器几何效率的值。本项目中称之为相对活度
    //反推出0时刻的计数。
    double A0_omiga = 0.0;
    if(Nc > 0) A0_omiga = N10 * N20 / Nco0 / f; //防止分母为0

    //-------------------------------------更新界面
    ui->lineEdit_particleNum->setText(QString::number(detectNum, 'E', 5));
    //修正前数据
    for(int i=0; i<3; i++){
        ui->tableWidget1->item(i, 0)->setText(QString::number(minCount[i]));
        ui->tableWidget1->item(i, 1)->setText(QString::number(maxCount[i]));
        ui->tableWidget1->item(i, 2)->setText(QString::number(countAverage[i]));
    }
    //死时间率，注意单位是%
    for(int i=3; i<6; i++){
        ui->tableWidget1->item(i, 0)->setText(QString::number(minDeathRatio[i-3]*100.0));
        ui->tableWidget1->item(i, 1)->setText(QString::number(maxDeathRatio[i-3]*100.0));
        ui->tableWidget1->item(i, 2)->setText(QString::number(deathRatioAverage[i-3]*100.0));
    }
    
    //修正后数据
    //Det1计数率
    ui->countRateDet1_min->setText(QString::number(minCount_correct[0]));
    ui->countRateDet1_max->setText(QString::number(maxCount_correct[0]));
    ui->countRateDet1_ave->setText(QString::number(countAverage_orrect[0]));
    //Det2计数率
    ui->countRateDet2_min->setText(QString::number(minCount_correct[1]));
    ui->countRateDet2_max->setText(QString::number(maxCount_correct[1]));
    ui->countRateDet2_ave->setText(QString::number(countAverage_orrect[1]));
    //符合计数率
    ui->countRateCoin_min->setText(QString::number(minCount_correct[2]));
    ui->countRateCoin_max->setText(QString::number(maxCount_correct[2]));
    ui->countRateCoin_ave->setText(QString::number(countAverage_orrect[2]));

    //总计数
    ui->lineEdit_totalCount1->setText(QString::number(N10));
    ui->lineEdit_totalCount2->setText(QString::number(N20));
    ui->lineEdit_totalCount3->setText(QString::number(Nco0));
    ui->lineEdit_realativeA0->setText(QString::number(A0_omiga, 'g', 9));

    activeOmigaToYield(A0_omiga);
}

void OfflineDataAnalysisWidget::on_pushbutton_save_clicked()
{
    //判断文件路径是否有效
    QString path = ui->lineEdit_savepath->text();
    if(!isValidFilePath(path)){
        // 信息提示框
        QMessageBox::information(nullptr, "提示", "保存失败，文件路径不存在或者不可用");
        return;
    }
    
    //判断文件是否已经存在，提示用户文件已存在。
    QString filename = ui->lineEdit_fileName->text();
    //自动转换文件后缀为.txt
    QString wholepath = smartAddTxtExtension(path + "/" + filename);
    if(fileExists(wholepath)){
        if (QMessageBox::question(this, tr("提示"), tr("保存文件名已经存在，是否覆盖重写？"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
        return;
    }
    QFile file(wholepath);
    if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QTextStream out(&file);
        //测量基本信息
        out << tr("测量基本信息:") << Qt::endl;
        out << tr("解析文件路径=") << ui->textBrowser_filepath->toPlainText()<< Qt::endl;
        out << tr("测量模式=") << ui->lineEdit_measuremodel->text()<< Qt::endl;
        out << tr("量程=") << ui->lineEdit_range->text()<< Qt::endl;
        out << tr("粒子数据个数=") << ui->lineEdit_particleNum->text()<< Qt::endl;
        out << tr("冷却时长(s)=") << ui->spinBox_coolingTime->value()<< Qt::endl;
        out << Qt::endl;

        //解析参数
        out << tr("解析参数:") << Qt::endl;
        out << tr("Det1符合能窗左=") << ui->spinBox_1_leftE->value() << Qt::endl;
        out << tr("Det1符合能窗右=") << ui->spinBox_1_rightE->value() << Qt::endl;
        out << tr("Det2符合能窗左=") << ui->spinBox_2_leftE->value() << Qt::endl;
        out << tr("Det2符合能窗右=") << ui->spinBox_2_rightE->value() << Qt::endl;
        out << tr("符合分辨时间/ns=") << ui->spinBox_timeWidth->value() << Qt::endl;
        out << tr("延迟时间/ns=") << ui->spinBox_delayTime->value() << Qt::endl;
        out << tr("开始时刻/s=") << ui->spinBox_start->value() << Qt::endl;
        out << tr("结束时刻/s=") << ui->spinBox_end->value() << Qt::endl;
        out << tr("时间步长/s=") << ui->spinBox_step->value() << Qt::endl;
        out << Qt::endl;

        //数据分析结果
        out << tr("数据分析结果:") << Qt::endl;
        out << tr("Det1计数率/cps: 最小值=") << ui->tableWidget1->item(0, 0)->text()
            << tr("，最大值=") << ui->tableWidget1->item(0, 1)->text()
            << tr("，平均值=") << ui->tableWidget1->item(0, 2)->text() << Qt::endl;
            
        out << tr("Det2计数率/cps: 最小值=") << ui->tableWidget1->item(1, 0)->text()
            << tr("，最大值=") << ui->tableWidget1->item(1, 1)->text()
            << tr("平均值=") << ui->tableWidget1->item(1, 2)->text() << Qt::endl;

        out << tr("符合计数率/cps: 最小值=") << ui->tableWidget1->item(2, 0)->text()
            << tr("，最大值=") << ui->tableWidget1->item(2, 1)->text()
            << tr("，平均值=") << ui->tableWidget1->item(2, 2)->text() << Qt::endl;
            
        out << tr("Det1死时间率(%): 最小值=") << ui->tableWidget1->item(3, 0)->text()
            << tr("，最大值=") << ui->tableWidget1->item(3, 1)->text()
            << tr("，平均值=") << ui->tableWidget1->item(3, 2)->text() << Qt::endl; 

        out << tr("Det2死时间率(%): 最小值=") << ui->tableWidget1->item(4, 0)->text()
            << tr("，最大值=") << ui->tableWidget1->item(4, 1)->text()
            << tr("，平均值=") << ui->tableWidget1->item(4, 2)->text() << Qt::endl;

        out << tr("符合计数死时间率(%): 最小值=") << ui->tableWidget1->item(5, 0)->text()
            << tr("，最大值=") << ui->tableWidget1->item(5, 1)->text()
            << tr("，平均值=") << ui->tableWidget1->item(5, 2)->text() << Qt::endl;
        out << Qt::endl;

        //死时间修正后
        out << tr("死时间修正后:") << Qt::endl;
        out << tr("Det1计数率/cps: 最小值=") << ui->countRateDet1_min->text()
            << tr("，最大值=") << ui->countRateDet1_max->text()
            << tr("，平均值=") << ui->countRateDet1_ave->text() << Qt::endl;

        out << tr("Det2计数率/cps: 最小值=") << ui->countRateDet2_min->text()
            << tr("，最大值=") << ui->countRateDet2_max->text()
            << tr("，平均值=") << ui->countRateDet2_ave->text() << Qt::endl;            

        out << tr("符合计数率/cps: 最小值=") << ui->countRateCoin_min->text()
            << tr("，最大值=") << ui->countRateCoin_max->text()
            << tr("，平均值=") << ui->countRateCoin_ave->text() << Qt::endl; 

        out << tr("Det1总计数=") << ui->lineEdit_totalCount1->text() << Qt::endl; 
        out << tr("Det2总计数=") << ui->lineEdit_totalCount2->text() << Qt::endl; 
        out << tr("总符合计数=") << ui->lineEdit_totalCount3->text() << Qt::endl;
        
        out << tr("相对初始活度/Bq=") << ui->lineEdit_realativeA0->text() << Qt::endl;
        out << tr("中子产额=") << ui->lineEdit_neutronYield->text() << Qt::endl;

        int yield_num = activationResult.size();
        if(yield_num>0 && isUpdateYield)
        {
            out<<tr("时间(s),相对初始活度,中子产额")<<Qt::endl;
            for (const auto& oneData : activationResult) {
                out << oneData.time << ","
                    << QString::number(oneData.ActiveOmiga, 'g', 9) << ","
                    <<QString::number(oneData.Yield, 'E', 6) << Qt::endl;
            }
        }
        file.flush();
        file.close();

        //保存计数率
        {
            wholepath = smartAddTxtExtension(path + "/" + filename + "_符合计数");
            file.setFileName(wholepath);
            if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
                QTextStream out(&file);
                //测量基本信息
                //out << tr("时间,Det-1 计数率/cps,Det-2 计数率/cps,符合结果 计数率/cps") << Qt::endl;
                out << "time,CountRate1,CountRate2,ConCount_single,ConCount_multiple,deathRatio1(%),deathRatio2(%)" << Qt::endl;
                vector<CoincidenceResult> coinResult = coincidenceAnalyzer->GetCoinResult();
                for (auto coin : coinResult){
                    if (coin.time >= startTimeUI && coin.time <= endTimeUI){
                        out << coin.time << "," << coin.CountRate1 << "," << coin.CountRate2 \
                            << "," << coin.ConCount_single << "," << coin.ConCount_multiple
                            << "," << coin.DeathRatio1 << "," << coin.DeathRatio2
                            << Qt::endl;
                    }
                }

                file.flush();
                file.close();
                QMessageBox::information(nullptr, "提示", "文件保存成功！");
            }
        }
    }
}

bool OfflineDataAnalysisWidget::isValidFilePath(const QString &path, bool checkExists, bool checkWritable) {
    // 空路径无效
    if (path.isEmpty()) return false;
    
    QFileInfo info(path);
    
    // 检查路径格式
    QDir dir(path);
    if (!dir.isAbsolute()) return false;
    
    // 检查路径是否存在(如果需要)
    if (checkExists && !info.exists()) return false;
    
    // 检查是否可写(如果需要)
    if (checkWritable && !info.isWritable()) return false;
    
    return true;
}

bool OfflineDataAnalysisWidget::fileExists(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    return fileInfo.exists() && fileInfo.isFile();
}


QString OfflineDataAnalysisWidget::smartAddTxtExtension(const QString &fileName) {
    QFileInfo fileInfo(fileName);
    
    // 获取文件后缀(不带点)
    QString suffix = fileInfo.suffix().toLower();
    
    // 如果后缀不是txt，则追加.txt
    if (suffix != "txt") {
        // 如果原始文件名已有其他后缀，则替换
        if (!suffix.isEmpty()) {
            return fileInfo.path() + "/" + fileInfo.completeBaseName() + ".txt";
        }
        // 如果原始文件名没有后缀，直接追加
        else {
            return fileName + ".txt";
        }
    }
    
    // 如果已经是.txt后缀，直接返回原文件名
    return fileName;
}

void OfflineDataAnalysisWidget::on_checkBox_gauss_stateChanged(int arg1)
{
    PlotWidget* plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
    plotWidget->slotShowGaussInfor(arg1 == Qt::CheckState::Checked);
}

void OfflineDataAnalysisWidget::loadConfig()
{
    QFile file(QApplication::applicationDirPath() + "/config/user.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();
        
        QJsonObject jsonPublic;
        if (jsonObj.contains("Public")){
            jsonPublic = jsonObj["Public"].toObject();
            if (jsonPublic.contains("deltaTime_updateYield")){
                deltaTime_updateYield = jsonPublic["deltaTime_updateYield"].toInt();
            }
            if (jsonPublic.contains("maxTime_updateYield")){
                maxTime_updateYield = jsonPublic["maxTime_updateYield"].toInt();
            }
            if (jsonPublic.contains("updateYield")){
                isUpdateYield = jsonPublic["updateYield"].toBool();
            }
        }
    }
    else{
        qCritical().noquote()<<"未找到拟合参数，请检查仪器是否进行了刻度,请对仪器刻度后重新计算（采用‘数据查看和分析’子界面分析）";
    }
}
