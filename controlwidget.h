#ifndef CONTROLWIDGET_H
#define CONTROLWIDGET_H

#include <QWidget>
#include "controlhelper.h"

namespace Ui {
class ControlWidget;
}

class ControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControlWidget(QWidget *parent = nullptr);
    ~ControlWidget();

    void load();
    void save();

    void enableUiStatus();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    virtual void closeEvent(QCloseEvent *event) override;

private slots:
    void on_pushButton_zero_clicked();

    void on_pushButton_stop_clicked();

    void on_pushButton_absolute_position_clicked();

    void on_pushButton_relative_position_clicked();

    void on_pushButton_speed_clicked();

    void on_pushButton_setup_clicked();

    void on_pushButton_start_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::ControlWidget *ui;
    ControlHelper *controlHelper = nullptr;
    qint32 mAxis_no = 0x01;
};

#endif // CONTROLWIDGET_H
