#include "equipmentmanagementform.h"
#include "ui_equipmentmanagementform.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QDebug>

EquipmentManagementForm::EquipmentManagementForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EquipmentManagementForm)
{
    ui->setupUi(this);

    //ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 平分

    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        QJsonObject jsonDetector;
        QJsonObject jsonRelay;
        QJsonObject jsonControl;
        if (jsonObj.contains("Detector")){
            jsonDetector = jsonObj["Detector"].toObject();
            ui->tableWidget->item(0, 0)->setText(jsonDetector["ip"].toString());
            ui->tableWidget->item(0, 1)->setText(QString::number(jsonDetector["port"].toInt()));
        }

        if (jsonObj.contains("Relay")){
            jsonRelay = jsonObj["Relay"].toObject();
            ui->tableWidget->item(1, 0)->setText(jsonRelay["ip"].toString());
            ui->tableWidget->item(1, 1)->setText(QString::number(jsonRelay["port"].toInt()));
        }

        if (jsonObj.contains("Control")){
            jsonControl = jsonObj["Control"].toObject();
            ui->tableWidget->item(2, 0)->setText(jsonControl["ip"].toString());
            ui->tableWidget->item(2, 1)->setText(QString::number(jsonControl["port"].toInt()));
        }
    }
}

EquipmentManagementForm::~EquipmentManagementForm()
{
    delete ui;
}

void EquipmentManagementForm::on_pushButton_ok_clicked()
{
    // 保存参数
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonObject jsonDetector;
        if (jsonObj.contains("Detector")){
            jsonDetector = jsonObj["Detector"].toObject();            
        }
        jsonDetector["ip"] = ui->tableWidget->item(0, 0)->text();
        jsonDetector["port"] = ui->tableWidget->item(0, 1)->text().toInt();
        jsonObj["Detector"] = jsonDetector;

        QJsonObject jsonRelay;
        if (jsonObj.contains("Relay")){
        }
        jsonRelay = jsonObj["Relay"].toObject();
        jsonRelay["ip"] = ui->tableWidget->item(1, 0)->text();
        jsonRelay["port"] = ui->tableWidget->item(1, 1)->text().toInt();
        jsonObj["Relay"] = jsonRelay;

        QJsonObject jsonControl;
        if (jsonObj.contains("Control")){
            jsonControl = jsonObj["Control"].toObject();
        }
        jsonControl["ip"] = ui->tableWidget->item(2, 0)->text();
        jsonControl["port"] = ui->tableWidget->item(2, 1)->text().toInt();
        jsonObj["Control"] = jsonControl;

        file.open(QIODevice::WriteOnly | QIODevice::Text);
        jsonDoc.setObject(jsonObj);
        QByteArray newJsonData = jsonDoc.toJson();
        qDebug() << newJsonData;
        file.write(newJsonData);
        file.close();
    }

    this->close();
}

void EquipmentManagementForm::on_pushButton_cancel_clicked()
{
    this->close();
}
