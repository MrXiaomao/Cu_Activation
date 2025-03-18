#ifndef CACHEDIRCONFIGWIDGET_H
#define CACHEDIRCONFIGWIDGET_H

#include <QWidget>
#include "commandhelper.h"

namespace Ui {
class CacheDirConfigWidget;
}

class CacheDirConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CacheDirConfigWidget(QWidget *parent = nullptr);
    ~CacheDirConfigWidget();

    void load();
    bool save();

private slots:
    void on_pushButton_ok_clicked();

    void on_pushButton_cancel_clicked();

private:
    Ui::CacheDirConfigWidget *ui;
    CommandHelper *commandhelper = nullptr;//网络指令
};

#endif // CACHEDIRCONFIGWIDGET_H
