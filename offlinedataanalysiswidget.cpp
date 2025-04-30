/*
 * @Author: MrPan
 * @Date: 2025-04-20 09:21:28
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-04-30 14:35:58
 * @Description: 离线数据分析
 */
#include "offlinedataanalysiswidget.h"
#include "ui_offlinedataanalysiswidget.h"
#include "plotwidget.h"
#include <QButtonGroup>
#include <iostream>
#include <QFileDialog>
#include <QAction>
#include <QToolButton>

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

    //ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

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

        connect(customPlotWidget, &PlotWidget::sigPausePlot, this, [=](bool pause){
            emit sigPausePlot(pause);
        });

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
void OfflineDataAnalysisWidget::openEnergyFile(QString filePath)
{
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

            ui->spinBox_step->setValue(configMap.value("时间步长").toInt());
            ui->spinBox_timeWidth->setValue(configMap.value("符合分辨时间").toInt());
            ui->spinBox_coolingTime->setValue(configMap.value("冷却时长").toInt());

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

    //读取历史数据重新进行运算
    PlotWidget* plotWidget = this->findChild<PlotWidget*>("offline-PlotWidget");
    plotWidget->slotStart();

    unsigned short EnWindow[4] = {(unsigned short)ui->spinBox_1_leftE->value(), (unsigned short)ui->spinBox_1_rightE->value(),
                               (unsigned short)ui->spinBox_2_leftE->value(), (unsigned short)ui->spinBox_2_rightE->value()};
    int delayTime = ui->spinBox_delayTime->value();//符合延迟时间,ns
    int timeWidth = ui->spinBox_timeWidth->value();//时间窗宽度，单位ns(符合分辨时间)
    QLiteThread *calThread = new QLiteThread();
    calThread->setObjectName("calThread");
    calThread->setWorkThreadProc([=](){
        coincidenceAnalyzer->initialize();

        QByteArray aDatas = validDataFileName.toLocal8Bit();
        vector<TimeEnergy> data1_2, data2_2;
#ifdef QT_NO_DEBUG        
        SysUtils::realQuickAnalyzeTimeEnergy((const char*)aDatas.data(), [&](DetTimeEnergy detTimeEnergy, bool eof, bool *interrupted){
#else
        SysUtils::realAnalyzeTimeEnergy((const char*)aDatas.data(), [&](DetTimeEnergy detTimeEnergy, bool eof, bool *interrupted){
#endif
            if (eof){
                if (interrupted)
                    emit sigEnd(true);
                else
                    emit sigEnd(false);

                return;
            }

            quint8 channel = detTimeEnergy.channel;
            while (!detTimeEnergy.timeEnergy.empty()){
                TimeEnergy timeEnergy = detTimeEnergy.timeEnergy.front();
                detTimeEnergy.timeEnergy.erase(detTimeEnergy.timeEnergy.begin());
                if (channel == 0x00){
                    data1_2.push_back(timeEnergy);
                } else {
                    data2_2.push_back(timeEnergy);
                }
            }

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
    {}

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

void OfflineDataAnalysisWidget::on_pushButton_start_clicked()
{
    emit sigStart();
}

void OfflineDataAnalysisWidget::slotEnd(bool interrupted)
{
    if (interrupted)
        QMessageBox::information(this, tr("提示"), tr("文件解析意外被终止！"));
    else
        QMessageBox::information(this, tr("提示"), tr("文件解析已顺利完成！"));
}
