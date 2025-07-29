#include "spectrumModel.h"
#include "ui_spectrumModel.h"
#include <QFileDialog>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QAction>
#include <QToolButton>
#include <QFileDialog>
#include <QElapsedTimer>
#include "globalsettings.h"

SpectrumModel::SpectrumModel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SpectrumModel)
    , measuring(false)
{
    ui->setupUi(this);

    commandhelper = CommandHelper::instance();

    //波形路径
    {
        QAction *action = ui->lineEdit_path->addAction(QIcon(":/resource/open.png"), QLineEdit::TrailingPosition);
        QToolButton* button = qobject_cast<QToolButton*>(action->associatedWidgets().last());
        button->setCursor(QCursor(Qt::PointingHandCursor));
        connect(button, &QToolButton::pressed, this, [=](){
            QString dir = QFileDialog::getExistingDirectory(this);
            ui->lineEdit_path->setText(dir);
        });
    }

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=](){
        QDateTime now = QDateTime::currentDateTime();
        qint64 tt = timerStart.secsTo(now);
        QTime time = QTime(0,0,0).addSecs(tt);
        ui->label_5->setText(time.toString("HH:mm:ss"));
    });

    this->load();
    ui->pushButton_save->setEnabled(false);
    ui->pushButton_start->setEnabled(commandhelper->isConnected());

    // 步长+自动修正
    ui->spinBox_3->setToolTip("请输入10的倍数（如10, 20, 30）");
    ui->spinBox_3->setSingleStep(10);
    connect(ui->spinBox_3, QOverload<int>::of(&QSpinBox::valueChanged), [=](int value) {
        if (value % 10 != 0) {
            ui->spinBox_3->blockSignals(true);
            ui->spinBox_3->setValue((value / 10) * 10);
            ui->spinBox_3->blockSignals(false);
        }
    });

    connect(commandhelper, &CommandHelper::sigMeasureStart, this, [=](qint8 mmode, qint8 tmode){
        measuring = true;

        timerStart = QDateTime::currentDateTime();
        ui->label_2->setText(timerStart.toString("yyyy-MM-dd HH:mm:ss"));
        timer->start(500);
        ui->pushButton_start->setEnabled(false);
        ui->pushButton_save->setEnabled(false);
        ui->pushButton_stop->setEnabled(true);
    });

    connect(commandhelper, &CommandHelper::sigMeasureStopSpectrum, this, [=](){
        timer->stop();
        measuring = false;
        ui->pushButton_stop->setEnabled(false);
        ui->pushButton_save->setEnabled(true);
        ui->pushButton_start->setEnabled(true);
    });
}

SpectrumModel::~SpectrumModel()
{
    disconnect(commandhelper, nullptr, this, nullptr);
    if (measuring){
        commandhelper->slotStopManualMeasure();
    }

    delete ui;
}

void SpectrumModel::load()
{
    JsonSettings* spectrumSettings = GlobalSettings::instance()->mSpectrumSettings;
    if (!spectrumSettings->isOpen())
        return;

    spectrumSettings->prepare();
    spectrumSettings->beginGroup();
    ui->comboBox->setCurrentIndex(spectrumSettings->value("WaveformPolarity").toInt());
    ui->comboBox_4->setCurrentIndex(spectrumSettings->value("DetectorGain").toInt());

    ui->spinBox->setValue(spectrumSettings->value("TriggerThold1").toInt());
    ui->spinBox_2->setValue(spectrumSettings->value("TriggerThold2").toInt());

    ui->spinBox_4->setValue(spectrumSettings->value("RefreshTimeLength").toInt());

    ui->lineEdit_path->setText(spectrumSettings->value("Path").toString());
    ui->lineEdit_filename->setText(spectrumSettings->value("FileName").toString());
    spectrumSettings->endGroup();
    spectrumSettings->finish();
}

bool SpectrumModel::save()
{
    JsonSettings* spectrumSettings = GlobalSettings::instance()->mSpectrumSettings;
    if (!spectrumSettings->isOpen())
        return false;

    // 保存参数
    spectrumSettings->prepare();
    spectrumSettings->beginGroup();

    //波形极性
    quint8 v = ui->comboBox->currentIndex();
    spectrumSettings->setValue("WaveformPolarity", v);

    //探测器增益
    {
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
        quint8 ch1 = 0x00;
        if (ui->comboBox_4->currentText() == "1.26"){
            ch1 = 0x04;
        } else if (ui->comboBox_4->currentText() == "2.52"){
            ch1 = 0x05;
        } else if (ui->comboBox_4->currentText() == "5.01"){
            ch1 = 0x06;
        } else {
            ch1 = 0x04;
        }
        spectrumSettings->setValue("DetectorGain", ch1);
    }

    //探测器1-2阈值
    {
        quint16 ch1 = (quint16)ui->spinBox->value();
        quint16 ch2 = (quint16)ui->spinBox_2->value();
        spectrumSettings->setValue("TriggerThold1", ch1);
        spectrumSettings->setValue("TriggerThold2", ch2);
    }

    //探测器3-4阈值
    {
        quint16 ch3 = 0x00;
        quint16 ch4 = 0x00;
        spectrumSettings->setValue("TriggerThold3", ch3);
        spectrumSettings->setValue("TriggerThold4", ch4);
    }

    //死时间
    {
        quint16 deadTime = ui->spinBox_3->value();
        spectrumSettings->setValue("DeadTime", deadTime);
    }

    //能谱刷新时间
    {
        quint16 refreshTimeLength = ui->spinBox_4->value();
        spectrumSettings->setValue("RefreshTimeLength", refreshTimeLength);
    }

    //路径
    {
        spectrumSettings->setValue("Path", ui->lineEdit_path->text());
    }

    //文件名
    {
        spectrumSettings->setValue("FileName", ui->lineEdit_filename->text());
    }

    spectrumSettings->endGroup();
    bool result = spectrumSettings->flush();
    spectrumSettings->finish();
    return result;
}

void SpectrumModel::on_pushButton_start_clicked()
{
    QAbstractButton *btn = qobject_cast<QAbstractButton*>(sender());
    if (!measuring){
        // 先保存参数
        if (!this->save()){
            return;
        }

        //手动测量
        DetectorParameter detectorParameter;
        detectorParameter.triggerThold1 = 0x81;
        detectorParameter.triggerThold2 = 0x81;
        detectorParameter.waveformPolarity = 0x00;
        detectorParameter.refreshTimeLength = 0x01;
        detectorParameter.gain = 0x00;
        detectorParameter.transferModel = 0x00;// 0x00-能谱 0x03-波形 0x05-符合模式
        detectorParameter.measureModel = mmManual;
        
        // 默认打开梯形成形
        detectorParameter.isTrapShaping = true;
        detectorParameter.TrapShape_risePoint = 20;
        detectorParameter.TrapShape_peakPoint = 20;
        detectorParameter.TrapShape_fallPoint = 20;
        detectorParameter.TrapShape_constTime1 = (int16_t)0.9558*65536;
        detectorParameter.TrapShape_constTime2 = (int16_t)0.9556*65536;
        detectorParameter.Threshold_baseLine = 20;

        JsonSettings* spectrumSettings = GlobalSettings::instance()->mSpectrumSettings;
        if (spectrumSettings->isOpen()){
            spectrumSettings->prepare();
            spectrumSettings->beginGroup();
            detectorParameter.triggerThold1 = spectrumSettings->value("TriggerThold1").toInt();
            detectorParameter.triggerThold2 = spectrumSettings->value("TriggerThold2").toInt();
            detectorParameter.waveformPolarity = spectrumSettings->value("WaveformPolarity").toInt();
            detectorParameter.refreshTimeLength = spectrumSettings->value("RefreshTimeLength").toInt();
            detectorParameter.gain = spectrumSettings->value("DetectorGain").toInt();
            spectrumSettings->endGroup();
            spectrumSettings->finish();
        }

        commandhelper->slotStartManualMeasure(detectorParameter);

        ui->pushButton_save->setEnabled(false);
        ui->pushButton_start->setEnabled(false);
        ui->pushButton_stop->setEnabled(true);

        QTimer::singleShot(3000, this, [=](){
            //指定时间未收到开始测量指令，则按钮恢复初始状态
            if (!measuring){
                ui->pushButton_start->setEnabled(true);
            }
        });
    }
}

void SpectrumModel::on_pushButton_save_clicked()
{
    //存储路径
    //存储文件名
    QString filename = ui->lineEdit_path->text() + "/" + ui->lineEdit_filename->text();
    if (!filename.endsWith(".dat"))
        filename += ".dat";
    QFileInfo fInfo(filename);
    if (QFileInfo::exists(filename)){
        if (QMessageBox::question(this, tr("提示"), tr("保存文件名已经存在，是否覆盖重写？"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
            return ;
    }

    // 保存参数
    JsonSettings* waveSettings = GlobalSettings::instance()->mUserSettings;
    if (waveSettings->isOpen()){
        waveSettings->prepare();
        waveSettings->beginGroup();
        QJsonObject jsonObj;
        waveSettings->setValue("Path", ui->lineEdit_path->text());
        waveSettings->setValue("FileName", filename);
        waveSettings->endGroup();
        waveSettings->flush();
        waveSettings->finish();

        commandhelper->exportFile(filename);
    } else {
        QMessageBox::warning(this, tr("提示"), tr("数据保存失败！"), QMessageBox::Ok, QMessageBox::Ok);
    }
}

void SpectrumModel::closeEvent(QCloseEvent *event)
{
    if (measuring){
        QMessageBox::warning(this, tr("提示"), tr("测量过程中，禁止关闭该窗口！"), QMessageBox::Ok, QMessageBox::Ok);
        event->ignore();
        return ;
    }

    event->accept();
}

void SpectrumModel::on_pushButton_stop_clicked()
{
    commandhelper->slotStopManualMeasure();
}

