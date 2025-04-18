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
class QCPItemCurve;
enum TracerType
{
    XAxisTracer,
    YAxisTracer,
    DataTracer
};

class PlotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);

    QCustomPlot *allocCustomPlot(QString objName, QWidget *parent = nullptr);
    void dispatchAdditionalTipFunction(QCustomPlot *customPlot);
    void dispatchAdditionalDragFunction(QCustomPlot *customPlot);

    void initCustomPlot();
    void initMultiCustomPlot();

    void switchToCountMode(bool isCountMode);
    void switchToLogarithmicMode(bool isLogarithmic);

    void resetAxisCoords();
    void rescalAxisCoords(QCustomPlot* customPlot);

    QCPGraph* getGraph(int index); //0-Det1 1-Det2 2=符合曲线 3-高斯曲线
    void resetPlotDatas(QCustomPlot* customPlot);//右键重设数据初始状态
    void updateTracerPosition(QCustomPlot*, double, double);
    void areaSelectFinished();

public slots:
    void slotPlotClick(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event);
    void slotBeforeReplot();
    void slotRestorePlot(QMouseEvent*);
    void slotUpdateEnTimeWidth(unsigned short* timeWidth);

    //批量刷新
    void slotUpdatePlotDatas(SingleSpectrum, vector<CoincidenceResult>, int refreshTime);
    //更新空数据
    void slotUpdatePlotNullData(int refreshTime);

    void slotStart();
    void slotResetPlot();
    void slotGauss(int leftE, int rightE);
    void slotShowGaussInfor(bool visible);

    void slotShowTracer(QMouseEvent *event);//显示跟踪器
    void slotShowTracerLine(QCustomPlot* customPlot, double key, double value);//显示跟踪线

    void slotCountRefreshTimelength();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void sigMouseDoubleClickEvent();
    void sigUpdateMeanValues(QString name, unsigned int minMean, unsigned int maxMean);
    void sigPausePlot(bool); //是否暂停图像刷新
    void sigAreaSelected();//拟合区域选择完成

private:
    bool showAxis = false;//是否显示轴线
    bool isDragging = false;
    bool allowAreaSelected = false;//是否允许鼠标拖选择区域
    bool isPressed = false;
    QPoint dragStart;

    QColor clrBackground = QColor(255, 255, 255);
    QColor clrLine = QColor(2, 115, 189, 200);
    QColor clrLine2 = QColor(189, 115, 2, 200);
    QColor clrRang = QColor(255, 0, 0, 255);
    QColor clrSelect = QColor(0, 0, 255, 200);
    QColor clrGauseCurve = QColor(0, 0, 255, 200);
    bool axisVisible = true;

    //计数模式下，坐标轴默认范围值
    qint32 COUNT_X_AXIS_LOWER[2];
    qint32 COUNT_X_AXIS_UPPER[2];
    qint32 COUNT_Y_AXIS_LOWER[2];
    qint32 COUNT_Y_AXIS_UPPER[2];
    //能谱模式下
    qint32 SPECTRUM_X_AXIS_LOWER;
    qint32 SPECTRUM_X_AXIS_UPPER;
    qint32 SPECTRUM_Y_AXIS_LOWER;
    qint32 SPECTRUM_Y_AXIS_UPPER;

    QVector<double> energyValues[2];//存储测量开始以后所有数据
    QMutex mutexRefreshPlot;

    QAction *timeAllAction;
    QAction *timeM10Action;
    QAction *timeM5Action;
    QAction *timeM3Action;
    QAction *timeM1Action;

    QAction *allowSelectAreaAction;//选择拟合区域
    QAction *resetPlotAction;//恢复图像状态
};

#endif // PLOTWIDGET_H
