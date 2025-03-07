#ifndef COOLINGTIMEWIDGET_H
#define COOLINGTIMEWIDGET_H

#include <QWidget>

namespace Ui {
class CoolingTimeWidget;
}

class CoolingTimeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CoolingTimeWidget(QWidget *parent = nullptr);
    ~CoolingTimeWidget();

signals:
    void sigAskTimeLength(qint32);
    void sigUpdateTimeLength(qint32);

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_update_timelength(qint32);

private:
    Ui::CoolingTimeWidget *ui;
};

#endif // COOLINGTIMEWIDGET_H
