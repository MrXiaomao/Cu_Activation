#include "FPGASetting.h"
#include "ui_FPGASetting.h"
#include "commandhelper.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>

FPGASetting::FPGASetting(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FPGASetting)
{
    ui->setupUi(this);

    this->load();
}

FPGASetting::~FPGASetting()
{
    delete ui;
}

void FPGASetting::on_pushButton_save_clicked()
{
    this->save();
    this->close();
}

bool FPGASetting::save()
{
    // 保存参数
    QJsonObject jsonObj;

    //波形极性
    quint8 v = ui->comboBox->currentIndex();
    jsonObj["WaveformPolarity"] = v;

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
        jsonObj["DetectorGain"] = ch1;
    }

    //探测器1-2阈值
    {
        quint16 ch1 = (quint16)ui->spinBox->value();
        quint16 ch2 = (quint16)ui->spinBox_2->value();
        jsonObj["TriggerThold1"] = ch1;
        jsonObj["TriggerThold2"] = ch2;
    }

    //探测器3-4阈值
    {
        quint16 ch3 = 0x00;
        quint16 ch4 = 0x00;
        jsonObj["TriggerThold3"] = ch3;
        jsonObj["TriggerThold4"] = ch4;
    }

    //死时间
    {
        quint16 dieTimelength = ui->spinBox_3->value();
        jsonObj["DieTimeLength"] = dieTimelength;
    }

    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/fpga.json");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument jsonDoc(jsonObj);
        file.write(jsonDoc.toJson());
        file.close();
    }
}

void FPGASetting::load()
{
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/fpga.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        // 将 JSON 数据解析为 QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        ui->comboBox->setCurrentIndex(jsonObj["WaveformPolarity"].toInt());
        ui->comboBox_4->setCurrentIndex(jsonObj["DetectorGain"].toInt());

        ui->spinBox->setValue(jsonObj["TriggerThold1"].toInt());
        ui->spinBox_2->setValue(jsonObj["TriggerThold2"].toInt());

        ui->spinBox_3->setValue(jsonObj["DieTimeLength"].toInt());
    } else {

    }
}
