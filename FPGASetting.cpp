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
        if (ui->comboBox_4->currentText() == "1.26"){
            ch1 = 0x04;
        } else if (ui->comboBox_4->currentText() == "2.52"){
            ch1 = 0x05;
        } else if (ui->comboBox_4->currentText() == "5.01"){
            ch1 = 0x06;
        } else {
            ch1 = 0x04;
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
        quint16 deadTime = ui->spinBox_3->value();
        jsonObj["DeadTime"] = deadTime;
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
        return true;
    }
    return false;
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
        ui->comboBox_4->setCurrentIndex(jsonObj["DetectorGain"].toInt()-4);//注意，这里增益当前只保留了04 05 06.故下标从04开始。

        ui->spinBox->setValue(jsonObj["TriggerThold1"].toInt());
        ui->spinBox_2->setValue(jsonObj["TriggerThold2"].toInt());

        ui->spinBox_3->setValue(jsonObj["DeadTime"].toInt());
    } else {

    }
}

void FPGASetting::on_pushButton_close_clicked()
{
    this->close();
}
