#include "equipmentmanagementform.h"
#include "ui_equipmentmanagementform.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QDebug>
#include "globalsettings.h"

EquipmentManagementForm::EquipmentManagementForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EquipmentManagementForm)
{
    ui->setupUi(this);

    //ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 平分

    JsonSettings* ipSettings = GlobalSettings::instance()->mIpSettings;
    ipSettings->prepare();
    ipSettings->beginGroup("Detector");
    ui->tableWidget->item(0, 0)->setText(ipSettings->value("ip").toString());
    ui->tableWidget->item(0, 1)->setText(QString::number(ipSettings->value("port").toInt()));
    ipSettings->endGroup();    

    ipSettings->beginGroup("Relay");
    ui->tableWidget->item(1, 0)->setText(ipSettings->value("ip").toString());
    ui->tableWidget->item(1, 1)->setText(QString::number(ipSettings->value("port").toInt()));
    ipSettings->endGroup();

    ipSettings->beginGroup("Control");
    ui->tableWidget->item(2, 0)->setText(ipSettings->value("ip").toString());
    ui->tableWidget->item(2, 1)->setText(QString::number(ipSettings->value("port").toInt()));
    ipSettings->endGroup();

    ipSettings->finish();
}

EquipmentManagementForm::~EquipmentManagementForm()
{
    delete ui;
}

void EquipmentManagementForm::on_pushButton_ok_clicked()
{
    JsonSettings* ipSettings = GlobalSettings::instance()->mIpSettings;
    ipSettings->prepare();
    ipSettings->beginGroup("Detector");
    ipSettings->setValue("ip", ui->tableWidget->item(0, 0)->text());
    ipSettings->setValue("port", ui->tableWidget->item(0, 1)->text());
    ipSettings->endGroup();

    ipSettings->beginGroup("Relay");
    ipSettings->setValue("ip", ui->tableWidget->item(1, 0)->text());
    ipSettings->setValue("port", ui->tableWidget->item(1, 1)->text());
    ipSettings->endGroup();

    ipSettings->beginGroup("Control");
    ipSettings->setValue("ip", ui->tableWidget->item(2, 0)->text());
    ipSettings->setValue("port", ui->tableWidget->item(2, 1)->text());
    ipSettings->endGroup();

    ipSettings->flush();
    ipSettings->finish();
    this->close();
}

void EquipmentManagementForm::on_pushButton_cancel_clicked()
{
    this->close();
}
