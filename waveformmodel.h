#ifndef WAVEFORMMODEL_H
#define WAVEFORMMODEL_H

#include <QWidget>

namespace Ui {
class WaveformModel;
}

class WaveformModel : public QWidget
{
    Q_OBJECT

public:
    explicit WaveformModel(QWidget *parent = nullptr);
    ~WaveformModel();

private:
    Ui::WaveformModel *ui;
};

#endif // WAVEFORMMODEL_H
