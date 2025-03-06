#include "waveformmodel.h"
#include "ui_waveformmodel.h"
#include "commandhelper.h"
#include <QAction>
#include <QToolButton>
#include <QFileDialog>

WaveformModel::WaveformModel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WaveformModel)
    , commandHelper(new CommandHelper(this))
{
    ui->setupUi(this);

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
}

WaveformModel::~WaveformModel()
{
    delete commandHelper;
    delete ui;   
}

void WaveformModel::on_pushButton_setup_clicked()
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

    //波形触发模式
    quint8 mode = ui->comboBox_2->currentIndex();
    commandHelper->slotWaveformTriggerMode(mode);

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

    //波形长度
    {
        /*
        00:64
        01:128
        02:256
        03:512
        04:1024
        默认1024
        */

        quint8 length = 0x00;
        if (ui->comboBox_3->currentText() == "64"){
            length = 0x00;
        } else if (ui->comboBox_3->currentText() == "128"){
            length = 0x01;
        } else if (ui->comboBox_3->currentText() == "256"){
            length = 0x02;
        } else if (ui->comboBox_3->currentText() == "512"){
            length = 0x03;
        } else if (ui->comboBox_3->currentText() == "1024"){
            length = 0x04;
        } else {
            length = 0x04;
        }

        commandHelper->slotWaveformLength(length);
    }

    //死时间
    {
        quint16 dieTimelength = ui->spinBox_3->value();
        commandHelper->slotSpectnumMode(dieTimelength);
    }
}

void WaveformModel::on_pushButton_start_clicked()
{

}
