/*
 * @Author: MaoXiaoqing
 * @Date: 2025-04-23 19:19:37
 * @LastEditors: 
 * @LastEditTime: 2025-04-24 13:06:04
 * @Description: 请填写简介
 */
#ifndef YIELDCALIBRATION_H
#define YIELDCALIBRATION_H
#include <QWidget>
#include <QMap>
#include <QTableWidget>
// 中子产额标定，也称为中子产额刻度
namespace Ui {
class YieldCalibration;
}

typedef struct calibrationPair
{
    double yield; //产额
    double active_t0; //零时刻相对活度
    // 可选：添加构造函数
    calibrationPair(double a = 0.0, double b = 0.0): yield(a), active_t0(b){}
}caliPair;

class YieldCalibration : public QWidget
{
    Q_OBJECT

public:
    explicit YieldCalibration(QWidget *parent = nullptr);
    ~YieldCalibration();

    void load();
    bool save();

protected:
    virtual void closeEvent(QCloseEvent *event) override;

private slots:
    void on_pushButton_OK_clicked();

    void on_pushButton_cancel_clicked();

private:
    Ui::YieldCalibration *ui;
    bool isTableModified(QTableWidget *table);
    QMap<int, caliPair> mapCalibration;
};

#endif // YIELDCALIBRATION_H
