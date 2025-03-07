#include "equipmentmanagementform.h"
#include "ui_equipmentmanagementform.h"

EquipmentManagementForm::EquipmentManagementForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EquipmentManagementForm)
{
    ui->setupUi(this);

    //ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 平分
}

EquipmentManagementForm::~EquipmentManagementForm()
{
    delete ui;
}
