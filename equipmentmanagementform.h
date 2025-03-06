#ifndef EQUIPMENTMANAGEMENTFORM_H
#define EQUIPMENTMANAGEMENTFORM_H

#include <QWidget>

namespace Ui {
class EquipmentManagementForm;
}

class EquipmentManagementForm : public QWidget
{
    Q_OBJECT

public:
    explicit EquipmentManagementForm(QWidget *parent = nullptr);
    ~EquipmentManagementForm();

private:
    Ui::EquipmentManagementForm *ui;
};

#endif // EQUIPMENTMANAGEMENTFORM_H
