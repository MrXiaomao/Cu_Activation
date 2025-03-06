#include "equipmentmanagementform.h"
#include "ui_equipmentmanagementform.h"

EquipmentManagementForm::EquipmentManagementForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EquipmentManagementForm)
{
    ui->setupUi(this);
}

EquipmentManagementForm::~EquipmentManagementForm()
{
    delete ui;
}
