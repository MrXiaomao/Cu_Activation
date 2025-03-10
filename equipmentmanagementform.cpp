#include "equipmentmanagementform.h"
#include "ui_equipmentmanagementform.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>

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
        QJsonObject jsonDisplacement;
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

        if (jsonObj.contains("Displacement")){
            QJsonArray jsonDisplacements = jsonObj["Displacement"].toArray();
            jsonDisplacement = jsonDisplacements.at(0).toObject();
            ui->tableWidget->item(2, 0)->setText(jsonDisplacement["ip"].toString());
            ui->tableWidget->item(2, 1)->setText(QString::number(jsonDisplacement["port"].toInt()));

            jsonDisplacement = jsonDisplacements.at(1).toObject();
            ui->tableWidget->item(3, 0)->setText(jsonDisplacement["ip"].toString());
            ui->tableWidget->item(3, 1)->setText(QString::number(jsonDisplacement["port"].toInt()));
        }

        QString ip = jsonObj["Detector"].toString();
        qint32 port = jsonObj["triggerThold2"].toInt();
    }
}

EquipmentManagementForm::~EquipmentManagementForm()
{
    delete ui;
}

void EquipmentManagementForm::on_pushButton_ok_clicked()
{
    // 保存参数
    QJsonObject jsonObj;

    QJsonObject jsonDetector;
    QJsonObject jsonRelay;

    jsonDetector["ip"] = ui->tableWidget->item(0, 0)->text();
    jsonDetector["port"] = ui->tableWidget->item(0, 1)->text().toInt();
    jsonObj.insert("Detector", jsonDetector);

    jsonRelay["ip"] = ui->tableWidget->item(1, 0)->text();
    jsonRelay["port"] = ui->tableWidget->item(1, 1)->text().toInt();
    jsonObj.insert("Relay", jsonRelay);

    QJsonObject jsonDisplacement1, jsonDisplacement2;
    jsonDisplacement1["ip"] = ui->tableWidget->item(2, 0)->text();
    jsonDisplacement1["port"] = ui->tableWidget->item(2, 1)->text().toInt();
    jsonDisplacement2["ip"] = ui->tableWidget->item(3, 0)->text();
    jsonDisplacement2["port"] = ui->tableWidget->item(3, 1)->text().toInt();

    QJsonArray jsonDisplacements;
    jsonDisplacements.append(jsonDisplacement1);
    jsonDisplacements.append(jsonDisplacement2);
    jsonObj.insert("Displacement", jsonDisplacements);

    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument jsonDoc(jsonObj);
        file.write(jsonDoc.toJson());
        file.close();
    }

    this->close();
}

void EquipmentManagementForm::on_pushButton_cancel_clicked()
{
    this->close();
}
