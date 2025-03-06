#ifndef SPECTRUMMODEL_H
#define SPECTRUMMODEL_H

#include <QWidget>

namespace Ui {
class SpectrumModel;
}

class SpectrumModel : public QWidget
{
    Q_OBJECT

public:
    explicit SpectrumModel(QWidget *parent = nullptr);
    ~SpectrumModel();

    void initCustomPlot();

private slots:
    void on_pushButton_start_clicked();

    void on_pushButton_save_clicked();

    void on_pushButton_gauss_clicked();

    void slotPlowWidgetDoubleClickEvent();

private:
    Ui::SpectrumModel *ui;
    bool measuring;
};

#endif // SPECTRUMMODEL_H
