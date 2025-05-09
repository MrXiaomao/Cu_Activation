#ifndef SPLASHWIDGET_H
#define SPLASHWIDGET_H

#include <QDialog>
//#include "QImageProgressBar.h"

namespace Ui {
class SplashWidget;
}

class SplashWidget : public QDialog
{
    Q_OBJECT

public:
    explicit SplashWidget(QWidget *parent = nullptr);
    ~SplashWidget();

    static SplashWidget *instance() {
        static SplashWidget splashWidget;
        return &splashWidget;
    }

    //设置提示信息
    void setInfo(const QString &info, int fontSizeMain = 15, int timeout = 0);
    void setInfo(const QString &info, bool allowClose, int fontSizeMain = 15, int timeout = 0);
    void setInfo(const QString &info, bool allowClose, bool showProgress, int fontSizeMain = 15, int timeout = 0);
    void updataProgress(unsigned long long value, unsigned long long maximum);

    Q_SIGNAL void sigCancel();

protected:
    //void showEvent(QShowEvent *);
    void closeEvent(QCloseEvent *);

private slots:
    void on_pushButton_cancel_clicked();

private:
    Ui::SplashWidget *ui;
    //QImageProgressBar *progressBar = nullptr;
    int timeout = 0;
    int currentSec = 0;
};

#endif // SPLASHWIDGET_H
