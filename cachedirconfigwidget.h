#ifndef CACHEDIRCONFIGWIDGET_H
#define CACHEDIRCONFIGWIDGET_H

#include <QWidget>
#include "commandhelper.h"
#include <QReadWriteLock>
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
    static QReadWriteLock *m_sLock;   //定义静态读写锁（方便所有当前线程类对象使用）
};

#endif // CACHEDIRCONFIGWIDGET_H
