#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QDockWidget>

class QCustomPlot;
class QCPItemText;
class QCPItemLine;
class QCPItemRect;
class QCPAbstractPlottable;
class PlotWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);

    void setName(QString name);
    void initCustomPlot();
    QCustomPlot *customPlotInstance() const;

public slots:
    void slotPlotClick(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event);
    void slotSelectionChanged();
    void slotBeforeReplot();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void sigMouseDoubleClickEvent();

private:
    QCustomPlot *customPlot;
    QCPItemText *titleTextTtem;// 显示图例名称
    QCPItemText *coordsTextItem; // 显示拟合信息
    QCPItemText *textTipItem;// 用于鼠标点击显示轴坐标值信息
    QCPItemLine *lineFlagItem;// 标记选择点
    QCPItemRect *dragRectItem;//框选

    bool showAxis = false;//是否显示轴线
    QString title;
    bool isDragging = false;
    bool isPressed = false;
    QPoint dragStart;
};

#endif // PLOTWIDGET_H
