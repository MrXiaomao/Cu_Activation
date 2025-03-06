#ifndef FPGASETTING_H
#define FPGASETTING_H

#include <QWidget>

namespace Ui {
class FPGASetting;
}

class FPGASetting : public QWidget
{
    Q_OBJECT

public:
    explicit FPGASetting(QWidget *parent = nullptr);
    ~FPGASetting();

private:
    Ui::FPGASetting *ui;
};

#endif // FPGASETTING_H
