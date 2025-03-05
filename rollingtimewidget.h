//RollingTimeWidget
#ifndef ROLLINGTIMEWIDGET_H
#define ROLLINGTIMEWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QDateTime>
#include <QDebug>


class RollingTimeWidget : public QWidget
{
    Q_OBJECT
    /* 将采用动画的对象属性注册到QT中去，否则动画将无法执行 */
    Q_PROPERTY(int posYShifting READ PosYShifting WRITE setPosYShifting)


public:
    explicit RollingTimeWidget(QWidget *parent = nullptr);
    ~RollingTimeWidget();


    /**
     * @brief setTimeRange 重新设置显示小时范围函数方法
     * @param int hour最小值
     * @param int hour最大值
     * @return void 无
     */
    void setTimeRange(int min,int max); //设置重新设置范围

    /**
     * @brief PosYShifting 获取偏移量
     * @return int posYShifting
     */
    int PosYShifting();

    /**
     * @brief setPosYShifting 设置偏移量
     * @param int 偏移量设置值
     * @return void 无
     */
    void setPosYShifting(int posYShifting);

    void setCurrTimeVal(int val);

protected:
    /**
     * @brief mousePressEvent 重写鼠标按压事件，鼠标拖动进行滚动
     * @param QMouseEvent* 鼠标按压事件
     * @return void 无
     */
    void mousePressEvent(QMouseEvent *event)            override;

    /**
     * @brief wheelEvent 重写滚轮事件，滚轮进行滚动
     * @param QWheelEvent* 滚轮事件
     * @return void 无
     */
    void wheelEvent(QWheelEvent *event)                 override;

    /**
     * @brief mouseMoveEvent 重写鼠标移动事件，鼠标移动进入滚动判决
     * @param QMouseEvent* 鼠标移动事件
     * @return void 无
     */
    void mouseMoveEvent(QMouseEvent *event)             override;

    /**
     * @brief mouseReleaseEvent 重写鼠标松开事件，鼠标松开进入结果判决
     * @param QMouseEvent* 鼠标松开事件
     * @return void 无
     */
    void mouseReleaseEvent(QMouseEvent *event)          override;

    /**
     * @brief paintEvent 重写绘图事件，对hour进行绘制
     * @param QPaintEvent* 绘图事件
     * @return void 无
     */
    void paintEvent(QPaintEvent *paint)                 override;

    /**
     * @brief drawNumber 画出滚动数字函数
     * @param QPaintEvent* 画笔类导入
     * @param int 数字值value
     * @param int 滚动偏移量offset
     * @return void 无
     */
    void drawNumber(QPainter &painter,int value,int offset);

    /**
     * @brief rollAnimation 滚动动画方法
     * @return void 无
     */
    void rollAnimation();

signals:
    void signal_update_cur_time_val(int val);

private:
    /* 最小取值 */
    int min_Range = 0;
    /* 最大取值 */
    int max_Range = 0;
    /* 字体显示大小 */
    int numberSize = 5;
    /* 鼠标移动时判决器 */
    bool rollingJudgment = false;
    /* 记录按压鼠标时Y方向的初始位置 */
    int currentPosY = 0;
    /* 取系统的小时时间作为滚动时间选择的初始值 */
    int currentTime = 0;
    /* 当下的Y方向上的偏移量 */
    int posYShifting = 0;
    /* 声明一个动画对象 */
    QPropertyAnimation rollingAni;
};


#endif // ROLLINGTIMEWIDGET_H

