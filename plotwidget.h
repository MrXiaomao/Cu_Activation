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
class PlotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);

    QCustomPlot *allocCustomPlot(QString objName, QWidget *parent = nullptr);
    void dispatchAdditionalFunction(QCustomPlot *customPlot);

    void initCustomPlot();
    void initMultiCustomPlot();

    void switchShowModel(bool refModel);
    void switchDataModel(bool log);

    void resetAxisCoords();
    void rescalAxisCoords(QCustomPlot* customPlot);

    QCPGraph* getGraph(int index); //0-Det1 1-Det2 2=符合曲线 3-高斯曲线
    void resetPlotDatas(QCustomPlot* customPlot);//右键重设数据初始状态

public slots:
    void slotPlotClick(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event);
    void slotBeforeReplot();
    void slotRestorePlot();

    //批量刷新
    void slotUpdatePlotDatas(vector<SingleSpectrum>, vector<CurrentPoint>, vector<CoincidenceResult>);

    void slotResetPlot();
    void slotGauss(int leftE, int rightE);


protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void sigMouseDoubleClickEvent();
    void sigUpdateMeanValues(QString name, unsigned int minMean, unsigned int maxMean);

private:    
    bool showAxis = false;//是否显示轴线
    bool isDragging = false;
    bool enableDrag = false;
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
};

#endif // PLOTWIDGET_H
