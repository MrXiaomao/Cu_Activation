#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QStackedWidget>
#include <QMutex>
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

class PlotWidget : public QStackedWidget
{
    Q_OBJECT

    enum Axis_model{
        amEnDet1 = 0x00,    //能谱1
        amEnDet2= 0x01,     //能谱2
        amCountDet1 = 0x02, //计数1
        amCountDet2 = 0x03, //计数2
        amCoResult = 0x04,  //符合计数

        amGaussEnDet1 = 0x10,    //高斯1
        amGaussEnDet2= 0x11,     //高斯2
    };

    enum Plot_Opt_model{
        pmNone = 0x00,
        pmMove = 0x01,
        pmTip = 0x04,
        pmDrag = 0x08
    };

public:
    explicit PlotWidget(QWidget *parent = nullptr);

    QCustomPlot *allocCustomPlot(QString objName, bool needGauss, QWidget *parent = nullptr);
    void dispatchAdditionalTipFunction(QCustomPlot *customPlot);
    void dispatchAdditionalDragFunction(QCustomPlot *customPlot);

    void initCustomPlot();
    void initMultiCustomPlot();

    void switchToCountMode(bool isCountModel);
    void switchToLogarithmicMode(bool isLogarithmic);

    //获取上一次拟合状态，true:拟合成功，false:拟合失败
    // bool getLastFitStatus();
    //向配置文件中写入上一次拟合状态。
    // bool setLastFitStatus();
    void switchToTipMode();//数据提示
    void switchToDragMode();//选择拟合区域
    void switchToMoveMode();//平移视图

    void resetAxisCoords();
    void rescalAxisCoords(QCustomPlot* customPlot);

    QList<QCustomPlot*> getAllEnergyCustomPlot();
    QList<QCustomPlot*> getAllCountCustomPlot();
    QList<QCustomPlot*> getAllCustomPlot();

    QCustomPlot* getCustomPlot(int index); //Axis_model
    QCPGraph* getGraph(int index); //Axis_model
    void resetPlotDatas(QCustomPlot* customPlot);//右键重设数据初始状态
    void updateTracerPosition(QCustomPlot*, double, double);
    void areaSelectFinished();


public slots:
    void slotBeforeReplot();
    void slotRestorePlot(QMouseEvent*);
    void slotUpdateEnWindow(unsigned short* EnWindow);
    void slotRestoreView();

    //批量刷新
    void slotUpdatePlotDatas(SingleSpectrum, vector<CoincidenceResult>);
    
    //能谱刷新整个数据，计数曲线增加一个数据点。
    void slotAddPlotDatas(SingleSpectrum r1, CoincidenceResult r3);
    
    //更新空数据
    void slotUpdatePlotNullData(int refreshTime);

    void slotStart(unsigned int channel = 8192);
    void slotResetPlot();
    void slotGauss(QCustomPlot* customPlot, int leftE, int rightE);
    void slotShowGaussInfor(bool visible);

    void slotShowTracer(QMouseEvent *event);//显示跟踪器，游标
    void slotShowTracerLine(QCustomPlot* customPlot, double key, double value);//显示跟踪线

    void slotCountRefreshTimelength();

    void slotHideDataTip();
protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void sigMouseDoubleClickEvent();
    void sigUpdateEnWindow(QString name, unsigned int left_En, unsigned int right_En);//更新能窗左右区间
    void sigPausePlot(bool); //是否暂停图像刷新
    void sigAreaSelected();//拟合区域选择完成

    void sigSwitchToTipMode();//数据提示
    void sigSwitchToDragMode();//选择拟合区域
    void sigSwitchToMoveMode();//平移视图

private:
    unsigned int multiChannel; //多道道数
    unsigned int max_UIChannel; //图像上最大道址，比多道道址略多一点，采用取整得到。
    bool showAxis = false;//是否显示轴线
    bool isDragging = false;
    bool allowAreaSelected = false;//是否允许鼠标拖选择区域
    bool isPressed = false;
    bool lastfitStatus = false;//上一次拟合是否成功
    QPoint dragStart;

    QColor clrBackground = QColor(255, 255, 255);
    QColor clrLine = QColor(2, 115, 189, 200);
    QColor clrLine2 = QColor(189, 115, 2, 200);
    QColor clrRang = QColor(255, 0, 0, 255);
    QColor clrSelect = QColor(0, 0, 255, 200);
    QColor clrGauseCurve = QColor(0, 0, 255, 200);
    bool axisVisible = true;

    //三条计数曲线,这种模式仅仅适合于数据点固定刷新时间间隔，测量途中不可切换。
    QVector<double> times, count1, count2, count3;
    QVector<QColor> colorsCount; //计数曲线数据点的颜色，三条曲线同样颜色
    double minCount[3], maxCount[3];

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
    qint32 SPECTURM_Y_AXIS_LOWER_LOG;

    QVector<double> energyValues[2];//存储测量开始以后所有数据
    QMutex mutexRefreshPlot;

    QAction *timeAllAction;
    QAction *timeM10Action;
    QAction *timeM5Action;
    QAction *timeM3Action;
    QAction *timeM1Action;

    QAction *allowSelectAreaAction;//选择拟合区域
    QAction *dataTipAction;//数据提示
    QAction *moveViewAction;//平移视图
    QAction *resetPlotAction;//恢复图像状态

    Plot_Opt_model mPlot_Opt_model = pmMove;
};

#endif // PLOTWIDGET_H
