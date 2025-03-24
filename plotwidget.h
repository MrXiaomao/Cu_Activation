#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QDockWidget>
#include <QMutex>
#include "sysutils.h"
#include "coincidenceanalyzer.h"

class QCustomPlot;
class QCPItemText;
class QCPItemLine;
class QCPItemRect;
class QCPGraph;
class QCPAbstractPlottable;
class QCPItemStraightLine;
class QCPItemCurve;
class PlotWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);

    void initCustomPlot();
    void initMultiCustomPlot();

    QCustomPlot *customPlotInstance() const;
    void initDetailWidget();

    void switchShowModel(bool refModel);
    void switchDataModel(bool log);

public slots:
    void slotPlotClick(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event);
    void slotSelectionChanged();
    void slotBeforeReplot();

//    void slotUpdateCountData(PariticalCountFrame frame); //计数
//    void slotUpdateSpectrumData(PariticalSpectrumFrame frame); //能谱

    //单次刷新
    void slotCoincidenceResult(quint32 time, CoincidenceResult);
    void slotSingleSpectrum(SingleSpectrum);
    void slotCurrentPoint(quint32 time, CurrentPoint);

    //批量刷新
    void slotCoincidenceResults(vector<CoincidenceResult>);
    void slotSingleSpectrums(vector<SingleSpectrum>);
    void slotCurrentPoints(vector<CurrentPoint>);

    void slotSingleSpectrumsAndCurrentPoints(quint8 channel, vector<SingleSpectrum>, vector<CurrentPoint>);

    void slotResetPlot();
    void slotGauss(int leftE, int rightE);

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void sigMouseDoubleClickEvent();
    void sigUpdateMeanValues(unsigned int minMean, unsigned int maxMean);

private:
    /*
     * 5个图形
     * 0-1 能谱图
     * 2 高斯曲线图
     * 3-4 计数图
    */
    enum {
        ENGRY_GRAPH = 0,
        REF_GRAPH = 1,
        GAUSS_GRAPH = 2,
        GRAPH_COUNT = 3
    };
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
    bool isLogarithmic = false;//对数模型
    QPoint dragStart;

    QColor clrBackground = QColor(255, 255, 255);
    QColor clrLine[2] = {QColor(0, 255, 0, 200), QColor(0, 255, 0, 200)};
    QColor clrRang = QColor(255, 0, 0, 255);
    QColor clrSelect = QColor(0, 0, 255, 200);
    QColor clrGauseCurve = QColor(0, 0, 255, 200);
    bool axisVisible = true;
    qint32 currentGraphIndex = 0;

    //PariticalSpectrumFrame currentFrame[GRAPH_COUNT];
    QMutex mutexRefreshPlot;
};

#endif // PLOTWIDGET_H
