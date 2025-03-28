#ifndef CONTROLWIDGET_H
#define CONTROLWIDGET_H

#include <QWidget>
#include "ftcoreimc.h"

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

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_pushButton_connect_clicked();

    void on_pushButton_zero_clicked();

    void on_pushButton_stop_clicked();

    void on_pushButton_absolute_position_clicked();

    void on_pushButton_relative_position_clicked();

    void on_pushButton_speed_clicked();

    void on_pushButton_setup_clicked();

private:
    Ui::ControlWidget *ui;
    FT_H mHandle1 = 0;
    FT_H mHandle2 = 0;
};

#endif // CONTROLWIDGET_H
