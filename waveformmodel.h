#ifndef WAVEFORMMODEL_H
#define WAVEFORMMODEL_H

#include <QWidget>

namespace Ui {
class WaveformModel;
}

class CommandHelper;
class WaveformModel : public QWidget
{
    Q_OBJECT

public:
    explicit WaveformModel(QWidget *parent = nullptr);
    ~WaveformModel();

private slots:
    void on_pushButton_setup_clicked();

    void on_pushButton_start_clicked();

private:
    Ui::WaveformModel *ui;
    CommandHelper *commandHelper;
};

#endif // WAVEFORMMODEL_H
