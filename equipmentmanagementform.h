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

private slots:
    void on_pushButton_ok_clicked();

    void on_pushButton_cancel_clicked();

private:
    Ui::EquipmentManagementForm *ui;
};

#endif // EQUIPMENTMANAGEMENTFORM_H
