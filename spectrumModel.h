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

private:
    Ui::SpectrumModel *ui;
};

#endif // SPECTRUMMODEL_H
