#ifndef WAVEFORMMODEL_H
#define WAVEFORMMODEL_H

#include <QWidget>
#include <QDateTime>
#include <QTimer>
#include <QCloseEvent>
#include "commandhelper.h"

namespace Ui {
class WaveformModel;
}

class QCloseEvent;
class WaveformModel : public QWidget
{
    Q_OBJECT

public:
    explicit WaveformModel(QWidget *parent = nullptr);
    ~WaveformModel();

    bool save();
    void load();

protected:
    virtual void closeEvent(QCloseEvent *event) override;

private slots:
    void on_pushButton_start_clicked();

    void on_pushButton_save_clicked();

    void on_pushButton_stop_clicked();

private:
    Ui::WaveformModel *ui;
    QTimer *timer;
    QDateTime timerStart;
    CommandHelper *commandhelper = nullptr;//网络指令
    bool measuring = false;
    qint64 total_filesize; //文件字节数
    qint32 total_ref = 0; //波形个数
};

#endif // WAVEFORMMODEL_H
