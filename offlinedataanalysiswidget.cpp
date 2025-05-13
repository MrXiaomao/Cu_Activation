/*
 * @Author: MrPan
 * @Date: 2025-04-20 09:21:28
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-05-13 09:39:05
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

    connect(this, &OfflineDataAnalysisWidget::sigPlot, this, [=](SingleSpectrum r1, vector<CoincidenceResult> r3){
        bool pause_plot = this->property("pause_plot").toBool();
        if (!pause_plot){
            PlotWidget* plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
            plotWidget->slotUpdatePlotDatas(r1, r3);

            // ui->lcdNumber_CountRate1->display(r3.back().CountRate1);
            // ui->lcdNumber_CountRate2->display(r3.back().CountRate2);
            // ui->lcdNumber_ConCount_single->display(r3.back().ConCount_single);
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
        this->doEnWindowData(r1, r3);
    });

    connect(this, SIGNAL(sigStart()), this, SLOT(slotStart()));
    connect(this, SIGNAL(sigEnd(bool)), this, SLOT(slotEnd(bool)), Qt::QueuedConnection);

    // 给出测量结果
    connect(this, &OfflineDataAnalysisWidget::sigActiveOmiga, this, [=](double active){
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
        ui->lineEdit_neutronYield->setText(QString::number(result, 'E', 5));
    });

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
    //this->setDockNestingEnabled(true);
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

        // connect(customPlotWidget, &PlotWidget::sigPausePlot, this, [=](bool pause){
        //     emit sigPausePlot(pause);
        // });

        connect(customPlotWidget, &PlotWidget::sigAreaSelected, this, [=](){
            // QWhatsThis::showText(ui->pushButton_confirm->mapToGlobal(ui->pushButton_confirm->geometry().bottomRight()), tr("需点击确认按钮，拟合区域才会设置生效"), ui->pushButton_confirm);
            // ui->pushButton_confirm->setIcon(QIcon(":/resource/warn.png"));
        });

        ui->widget_plot->layout()->addWidget(customPlotWidget);
    }
}

//#include <iostream>
#include "qlitethread.h"
#include <QFileInfo>
#include <QTextStream>
#include <QTimer>
void OfflineDataAnalysisWidget::openEnergyFile(QString filePath)
{
    //重新初始化
    for(int i=0; i<3; i++){
        for(int j=3; j<6; j++){
            ui->tableWidget1->item(j, i)->setText(QString::number(0));
        }
    }

    ui->textBrowser_filepath->setText(filePath);
    validDataFileName = filePath;

    QString configFile = validDataFileName + tr(".配置");
    if (QFileInfo::exists(configFile)){
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
                    configMap[key] = value;
                }
            }
            file.close();

            if(configMap.value("测量模式") == "手动") detParameter.measureModel = mmManual;
            else if(configMap.value("测量模式") == "自动") detParameter.measureModel = mmAuto;
            detParameter.coolingTime = configMap.value("冷却时长").toInt();
            detParameter.timeWidth = configMap.value("符合分辨时间").toInt();
            detParameter.measureRange = configMap.value("量程选取").toInt();
            this->startTime_FPGA = static_cast<unsigned int>(configMap.value("测量开始时间(FPGA时钟)").toInt());

            ui->lineEdit_measuremodel->setText(configMap.value("测量模式"));
            ui->spinBox_step->setValue(configMap.value("时间步长").toInt());
            ui->spinBox_timeWidth->setValue(configMap.value("符合分辨时间").toInt());
            ui->spinBox_coolingTime->setValue(configMap.value("冷却时长").toInt());
            ui->lineEdit_range->setText(configMap.value("量程选取"));
            
            ui->spinBox_1_leftE->setValue(configMap.value("Det1符合能窗左").toInt());
            ui->spinBox_1_rightE->setValue(configMap.value("Det1符合能窗右").toInt());
            ui->spinBox_2_leftE->setValue(configMap.value("Det2符合能窗左").toInt());
            ui->spinBox_2_rightE->setValue(configMap.value("Det1符合能窗右").toInt());
        }
    }
}

#include <QMessageBox>
void OfflineDataAnalysisWidget::slotStart()
{
    if (!analyzerFinished){
        QMessageBox::information(this, tr("提示"), tr("当前解析还未完成，请稍后重试！"));
        reAnalyzer = true;
        return;
    }

    firstPopup = true;
    reAnalyzer = false;
    detectNum = 0;
    lossData.clear();
    memset(&totalSingleSpectrum, 0, sizeof(totalSingleSpectrum));

    //读取历史数据重新进行运算
    PlotWidget* plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
    plotWidget->slotStart();

    unsigned short EnWindow[4] = {(unsigned short)ui->spinBox_1_leftE->value(), (unsigned short)ui->spinBox_1_rightE->value(),
                               (unsigned short)ui->spinBox_2_leftE->value(), (unsigned short)ui->spinBox_2_rightE->value()};
    int delayTime = ui->spinBox_delayTime->value();//符合延迟时间,ns
    int timeWidth = ui->spinBox_timeWidth->value();//时间窗宽度，单位ns(符合分辨时间)
    detParameter.timeWidth = timeWidth;

    QLiteThread *calThread = new QLiteThread();
    calThread->setObjectName("calThread");
    calThread->setWorkThreadProc([=](){
        coincidenceAnalyzer->initialize();

        if(detParameter.measureModel == mmManual){
            coincidenceAnalyzer->setCoolingTime_Manual(detParameter.coolingTime);
        }

        QByteArray aDatas = validDataFileName.toLocal8Bit();
        vector<TimeEnergy> data1_2, data2_2;
// #ifdef QT_NO_DEBUG
        SysUtils::realQuickAnalyzeTimeEnergy((const char*)aDatas.data(), [&](DetTimeEnergy detTimeEnergy, unsigned long long progress/*文件进度*/, unsigned long long filesize/*文件大小*/, bool eof, bool *interrupted){
// #else
//         SysUtils::realAnalyzeTimeEnergy((const char*)aDatas.data(), [&](DetTimeEnergy detTimeEnergy, bool eof, bool *interrupted){
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
            detectNum += detTimeEnergy.timeEnergy.size();

            if (channel == 0x00){
                data1_2 = detTimeEnergy.timeEnergy;
            } else if(channel == 0x01){
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
                coincidenceAnalyzer->calculate(data1_2, data2_2, (unsigned short*)EnWindow, timeWidth, delayTime, true, false);
                data1_2.clear();
                data2_2.clear();
            }

            *interrupted = reAnalyzer;
        });
    });
    calThread->start();
}

void OfflineDataAnalysisWidget::doEnWindowData(SingleSpectrum r1, vector<CoincidenceResult> r3)
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

        // 先不着急刷新图像和更新计数率，等数据全部处理完了再一次性刷新，避免耗时太久
        return;
    }

    //时间步长，求均值
    if (_stepT > 1){
        if (count>1 && (count % _stepT == 0)){
            vector<CoincidenceResult> rr3;

            for (size_t i=0; i < count/_stepT; i++){
                CoincidenceResult v;
                for (int j=0; j<_stepT; ++j){
                    size_t posI = i*_stepT + j;
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

            emit sigPlot(r1, rr3);
        }
    } else{
        vector<CoincidenceResult> rr3;
        for (size_t i=0; i < count; i++){
            rr3.push_back(r3[i]);
        }

        emit sigPlot(r1, rr3);
    }
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
    if (interrupted){
        SplashWidget::instance()->hide();
        QMessageBox::information(this, tr("提示"), tr("文件解析意外被终止！"));
    }
    else
    {
        SplashWidget::instance()->setInfo(tr("开始数据分析，请等待..."));
        
        //进行FPGA丢包的修正
        coincidenceAnalyzer->doFPGA_lossDATA_correction(lossData);
        
        // 读取活化测量的数据时刻区间[起始时间，结束时间]
        vector<CoincidenceResult> result = coincidenceAnalyzer->GetCoinResult();
        // int startTime = result.begin()->time;
        unsigned int startTime = this->startTime_FPGA;
        unsigned int endTime = result.back().time;

        //检查界面的起始时刻和结束时刻不超范围，若超范围则调整到范围内。
        unsigned int startTimeUI = static_cast<unsigned int>(ui->spinBox_start->value());
        unsigned int endTimeUI = static_cast<unsigned int>(ui->spinBox_end->value());

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

        analyse(detParameter, startTime, endTime);

        //在这里一次性更新图像和计数率，可以最大程度减小系统损耗时间
        {
            size_t count = result.size();
            int _stepT = ui->spinBox_step->value();

            //时间步长，求均值
            if (_stepT > 1){
                if (count>1 && (count % _stepT == 0)){
                    vector<CoincidenceResult> rr3;

                    for (size_t i=0; i < count/_stepT; i++){
                        CoincidenceResult v;
                        for (int j=0; j<_stepT; ++j){
                            size_t posI = i*_stepT + j;
                            v.CountRate1 += result[posI].CountRate1;
                            v.CountRate2 += result[posI].CountRate2;
                            v.ConCount_single += result[posI].ConCount_single;
                            v.ConCount_multiple += result[posI].ConCount_multiple;
                        }

                        //给出平均计数率cps,注意，这里是整除，当计数率小于1cps时会变成零。
                        v.CountRate1 /= _stepT;
                        v.CountRate2 /= _stepT;
                        v.ConCount_single /= _stepT;
                        v.ConCount_multiple /= _stepT;
                        rr3.push_back(v);
                    }

                    emit sigPlot(totalSingleSpectrum, rr3);
                }
            } else{
                vector<CoincidenceResult> rr3;
                for (size_t i=0; i < count; i++){
                    rr3.push_back(result[i]);
                }

                emit sigPlot(totalSingleSpectrum, rr3);
            }
        }

        SplashWidget::instance()->hide();
        QMessageBox::information(this, tr("提示"), tr("文件解析已顺利完成！"));
    }
}

/**
 * @description: 在计算出符合计数曲线之后，进行进一步分析，对与选定的时间区间进行数据处理。给出探测器1计数、探测器2计数、符合计数的最小值、最大值、平均值。
 * 以及死时间修正后的数值，探测器1总计数、探测器2总计数、总符合计数。
 * 计算出初始相对活度、中子产额。
 * 参考自CoincidenceAnalyzer::getInintialActive,注意离线处理产生的符合数据包含了冷却时间的零计数。与在线处理有一定区别
 * @param {DetectorParameter} detPara
 * @param {int} start_time
 * @param {int} time_end
 * @return {*}
 */
void OfflineDataAnalysisWidget::analyse(DetectorParameter detPara, unsigned int start_time, unsigned int time_end)
{
    vector<CoincidenceResult> coinResult = coincidenceAnalyzer->GetCoinResult();
    //如果没有前面的calculate的计算，那么后续计算不能进行
    if(coinResult.size() == 0) return ;
    
    size_t num = coinResult.size(); //计数点个数。

    //符合时间窗,单位ns
    int timeWidth_tmp = detPara.timeWidth;
    timeWidth_tmp = timeWidth_tmp/1e9;

    if(time_end < start_time) return ; //不允许起始时间小于停止时间

    int N1 = 0;
    int N2 = 0;
    int Nc = 0;
    double deathTime_Ratio1 = 0.0;
    double deathTime_Ratio2 = 0.0;
    
    //修正前的极值
    int minCount[3] = {100000, 100000, 100000}; //这里初值不能是一个常规小的计数率。
    int maxCount[3] = {0, 0, 0};
    double minDeathRatio[3] = {1.0, 1.0, 1.0}; //注意这里初值不能为0.0
    double maxDeathRatio[3] = {0.0, 0.0, 0.0};
    
    //修正后的极值
    double minCount_correct[3] = {10000.0, 10000.0, 10000.0}; //初值不能是一个小的数值
    double maxCount_correct[3] = {0.0, 0.0, 0.0};
    for(auto coin:coinResult)
    {
        // 手动测量，在改变能窗之前的数据不处理，注意剔除。现在数据没有保存这一段数据
        if(reAnalyzer){
            return;
        }

        if(coin.time > start_time && coin.time<time_end) {
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
            double n20 = n1 / (1-ratio2);
            double nc0 = nc / (1-ratio3);
            minCount_correct[0] = qMin(minCount_correct[0], n10);
            minCount_correct[1] = qMin(minCount_correct[1], n20);
            minCount_correct[2] = qMin(minCount_correct[2], nc0);

            maxCount_correct[0] = qMax(maxCount_correct[0], n10);
            maxCount_correct[1] = qMax(maxCount_correct[1], n20);
            maxCount_correct[2] = qMax(maxCount_correct[2], nc0);

            N1 += n1;
            N2 += n2;
            Nc += nc;
            deathTime_Ratio1 += ratio1;
            deathTime_Ratio2 += ratio2;
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
    // double T_halflife = 9.67*60; //单位s，Cu62的半衰期
    double T_halflife = 12.7*60*60; // 单位s,Cu64的半衰期
    double lamda = log(2) / T_halflife;
    double f = 1/lamda*(exp(-lamda*start_time) - exp(-lamda*time_end));

    //对符合计数进行真偶符合修正
    //注意timeWidth_tmp单位为ns，要换为时间s。
    double measureTime = (time_end - start_time)*1.0;
    double Nco = (Nc*measureTime - 2*timeWidth_tmp*N1*N2)/(measureTime - timeWidth_tmp*(N1+N2));

    //死时间修正
    double N10 = N1 / (1 - deathRatioAverage[0]);
    double N20 = N2 / (1 - deathRatioAverage[1]);
    double Nco0 = Nco / (1 - deathRatioAverage[0]) / (1 - deathRatioAverage[1]);
    //计算出活度乘以探测器几何效率的值。本项目中称之为相对活度
    //反推出0时刻的计数。

    double A0_omiga = N10 * N20 / Nco0 / f;

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
    ui->lineEdit_realativeA0->setText(QString::number(A0_omiga));

    emit sigActiveOmiga(A0_omiga);
}
