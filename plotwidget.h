#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QDockWidget>
#include "sysutils.h"

class QCustomPlot;
class QCPItemText;
class QCPItemLine;
class QCPItemRect;
class QCPGraph;
class QCPAbstractPlottable;
class QCPItemStraightLine;
class PlotWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);

    void initCustomPlot(int count);
    QCustomPlot *customPlotInstance() const;
    void initDetailWidget();

public slots:
    void slotPlotClick(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event);
    void slotSelectionChanged();
    void slotBeforeReplot();

    void slotUpdateCountData(PariticalCountFrame frame); //计数
    void slotUpdateSpectrumData(PariticalSpectrumFrame frame); //能谱

    void slotResetPlot();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void sigMouseDoubleClickEvent();

private:
    QCustomPlot *customPlot;
    QCPGraph *selectGraph = nullptr;
    QCPItemText *titleTextTtem;// 显示图例名称
    QCPItemText *coordsTextItem; // 显示拟合信息
    QCPItemText *textTipItem;// 用于鼠标点击显示轴坐标值信息
    QCPItemLine *lineFlagItem;// 标记选择点
    QCPItemRect *dragRectItem;//框选
    QCPItemStraightLine *lineLeft = nullptr;
    QCPItemStraightLine *lineRight = nullptr;

    bool showAxis = false;//是否显示轴线
    bool isDragging = false;
    bool isPressed = false;
    QPoint dragStart;

    QColor clrBackground = QColor(255, 255, 255);
    QColor clrLine[2] = {Qt::green, Qt::blue};
    QColor clrRang = QColor(255, 0, 0);
    QColor clrSelect = QColor(0, 0, 255);
    bool axisVisible = true;
    qint32 currentGraphIndex = 0;

    PariticalSpectrumFrame currentFrame[2];
};

#endif // PLOTWIDGET_H
