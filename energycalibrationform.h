#ifndef ENERGYCALIBRATIONFORM_H
#define ENERGYCALIBRATIONFORM_H

#include <QWidget>

namespace Ui {
class EnergyCalibrationForm;
}

class QCustomPlot;
class EnergyCalibrationForm : public QWidget
{
    Q_OBJECT

public:
    explicit EnergyCalibrationForm(QWidget *parent = nullptr);
    ~EnergyCalibrationForm();

private:
    void initCustomPlot();

private:
    Ui::EnergyCalibrationForm *ui;
    QCustomPlot* customPlot;
};

#endif // ENERGYCALIBRATIONFORM_H
