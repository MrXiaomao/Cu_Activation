#ifndef SPECTRUMMODEL_H
#define SPECTRUMMODEL_H

#include <QWidget>
#include <QDateTime>
#include <QTimer>
#include <QCloseEvent>
#include "commandhelper.h"

namespace Ui {
class SpectrumModel;
}

class QCloseEvent;
class SpectrumModel : public QWidget
{
    Q_OBJECT

public:
    explicit SpectrumModel(QWidget *parent = nullptr);
    ~SpectrumModel();

    bool save();
    void load();

protected:
    virtual void closeEvent(QCloseEvent *event) override;

private slots:
    void on_pushButton_start_clicked();

    void on_pushButton_save_clicked();

private:
    Ui::SpectrumModel *ui;
    QTimer *timer;
    QDateTime timerStart;
    CommandHelper *commandhelper = nullptr;//网络指令
    bool measuring = false;
};

#endif // SPECTRUMMODEL_H
