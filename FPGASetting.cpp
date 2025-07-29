#include "FPGASetting.h"
#include "ui_FPGASetting.h"
#include "commandhelper.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include "globalsettings.h"

FPGASetting::FPGASetting(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FPGASetting)
{
    ui->setupUi(this);

    this->load();

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
}

FPGASetting::~FPGASetting()
{
    delete ui;
}

void FPGASetting::on_pushButton_save_clicked()
{
    if (this->save()){
        // CommandHelper* commandHelper =CommandHelper::instance();
        QMessageBox::information(nullptr, tr("提示"), tr("硬件参数设置保存成功！"));
    }
    else{
        QMessageBox::information(nullptr, tr("提示"), tr("硬件参数设置保存失败,请重新尝试！"));
        return;
    }
    this->close();
}

bool FPGASetting::save()
{
    // 保存参数
    JsonSettings* fpgaSettings = GlobalSettings::instance()->mFpgaSettings;
    fpgaSettings->prepare();
    fpgaSettings->beginGroup();

    //波形极性
    quint8 v = ui->comboBox->currentIndex();
    fpgaSettings->setValue("WaveformPolarity", ui->comboBox->currentIndex());

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
        fpgaSettings->setValue("DetectorGain", ch1);
    }

    //探测器1-2阈值
    {
        quint16 ch1 = (quint16)ui->spinBox->value();
        quint16 ch2 = (quint16)ui->spinBox_2->value();
        fpgaSettings->setValue("TriggerThold1", ch1);
        fpgaSettings->setValue("TriggerThold2", ch1);
    }

    //探测器3-4阈值
    {
        quint16 ch3 = 0x00;
        quint16 ch4 = 0x00;
        fpgaSettings->setValue("TriggerThold3", ch3);
        fpgaSettings->setValue("TriggerThold4", ch4);
    }

    //死时间
    {
        quint16 deadTime = ui->spinBox_3->value();
        fpgaSettings->setValue("DeadTime", deadTime);
    }

    fpgaSettings->endGroup();
    bool result = fpgaSettings->flush();
    fpgaSettings->finish();
    return result;
}

void FPGASetting::load()
{
    JsonSettings* fpgaSettings = GlobalSettings::instance()->mFpgaSettings;
    if (!fpgaSettings->isOpen())
        return;

    fpgaSettings->prepare();
    fpgaSettings->beginGroup();
    ui->comboBox->setCurrentIndex(fpgaSettings->value("WaveformPolarity").toInt());
    ui->comboBox_4->setCurrentIndex(fpgaSettings->value("DetectorGain").toInt()-4);//注意，这里增益当前只保留了04 05 06.故下标从04开始。
    ui->spinBox->setValue(fpgaSettings->value("TriggerThold1").toInt());
    ui->spinBox_2->setValue(fpgaSettings->value("TriggerThold2").toInt());
    ui->spinBox_3->setValue(fpgaSettings->value("DeadTime").toInt());
    fpgaSettings->endGroup();
    fpgaSettings->finish();
}

void FPGASetting::on_pushButton_close_clicked()
{
    this->close();
}
