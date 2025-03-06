#include "FPGASetting.h"
#include "ui_FPGASetting.h"
#include "commandhelper.h"

FPGASetting::FPGASetting(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FPGASetting)
    , commandHelper(new CommandHelper(this))
{
    ui->setupUi(this);
}

FPGASetting::~FPGASetting()
{
    delete ui;
}

void FPGASetting::on_pushButton_setup_clicked()
{
    //波形极性
    quint8 v = ui->comboBox->currentIndex();
    commandHelper->slotWaveformPolarity(v);

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
        if (ui->comboBox_4->currentText() == "0.08"){
            ch1 = 0x00;
        } else if (ui->comboBox_4->currentText() == "0.16"){
            ch1 = 0x01;
        } else if (ui->comboBox_4->currentText() == "0.32"){
            ch1 = 0x02;
        } else if (ui->comboBox_4->currentText() == "0.63"){
            ch1 = 0x03;
        } else if (ui->comboBox_4->currentText() == "1.26"){
            ch1 = 0x04;
        } else if (ui->comboBox_4->currentText() == "2.52"){
            ch1 = 0x05;
        } else if (ui->comboBox_4->currentText() == "5.01"){
            ch1 = 0x06;
        } else if (ui->comboBox_4->currentText() == "10"){
            ch1 = 0x07;
        } else {
            ch1 = 0x00;
        }
        commandHelper->slotDetectorGain(ch1, ch1, ch1, ch1);
    }

    //探测器1阈值
    {
        quint16 ch1 = (quint16)ui->spinBox->value();
        quint16 ch2 = (quint16)ui->spinBox_2->value();
        commandHelper->slotTriggerThold1(ch1, ch2);
    }

    //探测器2阈值
    {
        quint16 ch3 = 0x00;
        quint16 ch4 = 0x00;
        commandHelper->slotTriggerThold2(ch3, ch4);
    }

    //死时间
    {
        quint16 dieTimelength = ui->spinBox_3->value();
        commandHelper->slotSpectnumMode(dieTimelength);
    }
}
