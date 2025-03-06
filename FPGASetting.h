#ifndef FPGASETTING_H
#define FPGASETTING_H

#include <QWidget>

namespace Ui {
class FPGASetting;
}

class CommandHelper;
class FPGASetting : public QWidget
{
    Q_OBJECT

public:
    explicit FPGASetting(QWidget *parent = nullptr);
    ~FPGASetting();

private slots:
    void on_pushButton_setup_clicked();

private:
    Ui::FPGASetting *ui;
    CommandHelper *commandHelper;
};

#endif // FPGASETTING_H
