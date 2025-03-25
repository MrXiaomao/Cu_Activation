#include "plotwidget.h"
#include "qcustomplot.h"
#include <QGridLayout>
#include <QtMath>
#include <cmath>

#define RANGE_SCARRE_UPPER 0.8
#define RANGE_SCARRE_LOWER 0.4
#define X_AXIS_LOWER    0
#define Y_AXIS_LOWER    -100

PlotWidget::PlotWidget(QWidget *parent) : QWidget(parent)
    , gaussResultItemText(nullptr)
    , coordsTipItemText(nullptr)
    , flagItemLine(nullptr)
{
    this->setContentsMargins(0, 0, 0, 0);
}

void PlotWidget::initCustomPlot(){
    QSplitter *spPlotWindow = new QSplitter(Qt::Vertical, this);
    //const qint32 titleHeight = 12;
    {
        QWidget *containWidget = new QWidget(this);
        containWidget->setLayout(new QVBoxLayout());
        containWidget->layout()->setContentsMargins(0, 0, 0, 0);
        QLabel* label = new QLabel("Det1");
        label->setContentsMargins(9, 0, 0, 0);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        //label->setFixedHeight(titleHeight);
        //label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        containWidget->layout()->addWidget(label);
        QCustomPlot *customPlot = allocCustomPlot("Det1", spPlotWindow);
        containWidget->layout()->addWidget(customPlot);
        spPlotWindow->addWidget(containWidget);
        customPlot->installEventFilter(this);

        dispatchAdditionalFunction(customPlot);
        customPlot->replot();
    }

    {
        QWidget *containWidget = new QWidget(this);
        containWidget->setLayout(new QVBoxLayout());
        containWidget->layout()->setContentsMargins(0, 0, 0, 0);
        QLabel* label = new QLabel("Det2");
        label->setContentsMargins(9, 0, 0, 0);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        //label->setFixedHeight(titleHeight);
        //label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        containWidget->layout()->addWidget(label);
        QCustomPlot *customPlot = allocCustomPlot("Det2", spPlotWindow);
        containWidget->layout()->addWidget(customPlot);
        spPlotWindow->addWidget(containWidget);
        customPlot->installEventFilter(this);

        dispatchAdditionalFunction(customPlot);
        customPlot->replot();
    }

    {
        QWidget *containWidget = new QWidget(this);
        containWidget->setLayout(new QVBoxLayout());
        containWidget->layout()->setContentsMargins(0, 0, 0, 0);
        QLabel* label = new QLabel("符合结果");
        label->setContentsMargins(9, 0, 0, 0);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        //label->setFixedHeight(titleHeight);
        //label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        containWidget->layout()->addWidget(label);
        QCustomPlot *customPlot = allocCustomPlot("CoincidenceResult", spPlotWindow);
        containWidget->layout()->addWidget(customPlot);
        spPlotWindow->addWidget(containWidget);
        customPlot->replot();
    }

    spPlotWindow->setStretchFactor(0, 3);
    spPlotWindow->setStretchFactor(1, 3);
    spPlotWindow->setStretchFactor(2, 2);

    this->setLayout(new QVBoxLayout(this));
    this->layout()->setContentsMargins(0, 5, 0, 0);
    this->layout()->addWidget(spPlotWindow);
}

void PlotWidget::dispatchAdditionalFunction(QCustomPlot *customPlot)
{
    // 文本元素随坐标变动而变动
    QCPItemText* gaussResultItemText = new QCPItemText(customPlot);
    gaussResultItemText->setObjectName("gaussResultItemText");
    gaussResultItemText->setColor(QColor(0,128,128,255));// 字体色
    gaussResultItemText->setPen(QColor(130,130,130,255));// 边框
    gaussResultItemText->setBrush(QBrush(QColor(255,255,225,255)));// 背景色
    gaussResultItemText->setPositionAlignment(Qt::AlignBottom | Qt::AlignLeft);
    gaussResultItemText->position->setType(QCPItemPosition::ptPlotCoords);
    gaussResultItemText->setTextAlignment(Qt::AlignLeft);
    gaussResultItemText->setFont(QFont(font().family(), 12));
    gaussResultItemText->setPadding(QMargins(5, 5, 5, 5));
    gaussResultItemText->position->setCoords(250.0, 1000.0);//X、Y轴坐标值
    gaussResultItemText->setText("");
    gaussResultItemText->setVisible(false);

    QCPItemText *coordsTipItemText = new QCPItemText(customPlot);
    coordsTipItemText->setObjectName("coordsTipItemText");
    coordsTipItemText->setPositionAlignment(Qt::AlignBottom|Qt::AlignLeft);
    coordsTipItemText->position->setType(QCPItemPosition::ptAbsolute);
    coordsTipItemText->setFont(QFont(font().family(), 15));
    coordsTipItemText->setPen(QPen(Qt::black));
    coordsTipItemText->setBrush(Qt::white);
    coordsTipItemText->setVisible(false);

    QCPItemLine *flagItemLine = new QCPItemLine(customPlot);
    flagItemLine->setObjectName("flagItemLine");
    flagItemLine->setAntialiased(false);
    flagItemLine->start->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    flagItemLine->end->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    flagItemLine->setPen(QPen(QColor(255,0,255,255), 3, Qt::SolidLine));
    flagItemLine->setSelectedPen(QPen(QColor(255,0,255,255), 3, Qt::SolidLine));
    flagItemLine->setVisible(false);

    QCPItemRect *dragRectItem = new QCPItemRect(customPlot);
    dragRectItem->setObjectName("dragRectItem");
    dragRectItem->setAntialiased(false);
    dragRectItem->topLeft->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    dragRectItem->bottomRight->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    dragRectItem->setPen(QPen(clrSelect, 2, Qt::DotLine));// 边框
    dragRectItem->setBrush(Qt::NoBrush);// 背景色
    dragRectItem->setSelectedPen(QPen(clrSelect, 2, Qt::DotLine));
    dragRectItem->setVisible(false);
}

QCPItemStraightLine *PlotWidget::allocStraightLineItem(QCustomPlot *customPlot)
{
    QCPItemStraightLine *line = new QCPItemStraightLine(customPlot);
    line->setLayer("overlay");
    line->setPen(QPen(Qt::gray, 1, Qt::DashLine));
    line->setClipToAxisRect(true);
    line->point1->setCoords(0, 0);
    line->point2->setCoords(0, 0);
    line->setVisible(false);
    return line;
}

QCustomPlot *PlotWidget::allocCustomPlot(QString objName, QWidget *parent)
{
    QWidget *dockWidgetContents = new QWidget();
    dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
    QGridLayout *gridLayout = new QGridLayout(dockWidgetContents);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    gridLayout->setContentsMargins(0, 0, 0, 0);

    QCustomPlot* customPlot = new QCustomPlot(parent);
    customPlot->setObjectName(objName);
    //customPlot->setOpenGl(true); //不能启用硬件加速，否则多个控件数据刷新起冲突
//    customPlot->installEventFilter(this);
//    customPlot->plotLayout()->insertRow(0);
//    customPlot->plotLayout()->addElement(0, 0, new QCPTextElement(customPlot, title, QFont("微软雅黑", 10, QFont::Bold)));
    // 设置选择容忍度，即鼠标点击点到数据点的距离
    //customPlot->setSelectionTolerance(5);
    // 设置全局抗锯齿
    //customPlot->setAntialiasedElements(QCP::aeAll);
    customPlot->setNotAntialiasedElements(QCP::aeAll);
    // 图例名称隐藏
    customPlot->legend->setVisible(false);
    customPlot->legend->setFillOrder(QCPLayoutGrid::foColumnsFirst);//设置图例在一行中显示
    // 图例名称显示位置
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignRight);
    // 设置边界
    customPlot->setContentsMargins(0, 0, 0, 0);
    // 设置标签倾斜角度，避免显示不下
    customPlot->xAxis->setTickLabelRotation(-45);
    // 背景色
    customPlot->setBackground(QBrush(clrBackground));
    // 图像画布边界
    customPlot->axisRect()->setMinimumMargins(QMargins(0, 0, 0, 0));
    // 坐标背景色
    customPlot->axisRect()->setBackground(clrBackground);
    // 允许拖拽，缩放
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    // 允许轴自适应大小
    //customPlot->graph(0)->rescaleValueAxis(true);
    customPlot->xAxis->rescale(false);
    customPlot->yAxis->rescale(false);
    // 设置刻度范围
    customPlot->xAxis->setRange(0, 180);
    customPlot->yAxis->setRange(Y_AXIS_LOWER, 10000);
    customPlot->yAxis->ticker()->setTickCount(5);
    customPlot->xAxis->ticker()->setTickCount(16);
    // 设置刻度可见
    customPlot->xAxis->setTicks(axisVisible);
    customPlot->xAxis2->setTicks(false);
    customPlot->yAxis->setTicks(axisVisible);
    customPlot->yAxis2->setTicks(false);
    // 设置轴线可见
    customPlot->xAxis->setVisible(axisVisible);
    customPlot->xAxis2->setVisible(axisVisible);
    customPlot->yAxis->setVisible(axisVisible);
    customPlot->yAxis2->setVisible(axisVisible);
    //customPlot->axisRect()->setupFullAxesBox();//四边安装轴并显示
    // 设置刻度标签可见
    customPlot->xAxis->setTickLabels(axisVisible);
    customPlot->xAxis2->setTickLabels(false);
    customPlot->yAxis->setTickLabels(axisVisible);
    customPlot->yAxis2->setTickLabels(false);
    // 设置子刻度可见
    customPlot->xAxis->setSubTicks(axisVisible);
    customPlot->xAxis2->setSubTicks(false);
    customPlot->yAxis->setSubTicks(axisVisible);
    customPlot->yAxis2->setSubTicks(false);
    //设置轴标签名称
    //customPlot->xAxis->setLabel(QObject::tr("时间"));
    //customPlot->yAxis->setLabel(QObject::tr("能量/Kev"));
    // 设置网格线颜色
    customPlot->xAxis->grid()->setPen(QPen(QColor(180, 180, 180, 128), 1, Qt::PenStyle::DashLine));
    customPlot->yAxis->grid()->setPen(QPen(QColor(180, 180, 180, 128), 1, Qt::PenStyle::DashLine));
    customPlot->xAxis->grid()->setSubGridPen(QPen(QColor(50, 50, 50, 128), 1, Qt::DotLine));
    customPlot->yAxis->grid()->setSubGridPen(QPen(QColor(50, 50, 50, 128), 1, Qt::DotLine));
    customPlot->xAxis->grid()->setZeroLinePen(QPen(QColor(50, 50, 50, 100), 1, Qt::SolidLine));
    customPlot->yAxis->grid()->setZeroLinePen(QPen(QColor(50, 50, 50, 100), 1, Qt::SolidLine));
    // 设置网格线是否可见
    customPlot->xAxis->grid()->setVisible(false);
    customPlot->yAxis->grid()->setVisible(false);
    // 设置子网格线是否可见
    customPlot->xAxis->grid()->setSubGridVisible(false);
    customPlot->yAxis->grid()->setSubGridVisible(false);

    // 添加散点图
    QCPGraph * curGraph = customPlot->addGraph(customPlot->xAxis, customPlot->yAxis);
    curGraph->setAntialiased(false);
    curGraph->setPen(QPen(clrLine));
    curGraph->selectionDecorator()->setPen(QPen(clrLine));
    curGraph->setLineStyle(QCPGraph::lsNone);// 隐藏线性图
    curGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 2));//显示散点图

    connect(customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*, int, QMouseEvent*)), this, SLOT(slotPlotClick(QCPAbstractPlottable*, int, QMouseEvent*)));
    connect(customPlot, SIGNAL(beforeReplot()), this, SLOT(slotBeforeReplot()));
    connect(customPlot, SIGNAL(afterLayout()), this, SLOT(slotBeforeReplot()));

    connect(customPlot->xAxis, QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this, [=](const QCPRange &range){
        qint64 maxRange = range.upper - range.lower;
        if (range.lower < 0){
           customPlot->xAxis->setRange(0, maxRange);
        } else if (maxRange < 1e2){
            customPlot->xAxis->setRange(range.lower, range.lower + 1e2);//0.01~1000
         }
    });
    connect(customPlot->yAxis, QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this, [=](const QCPRange &range){
        qint64 maxRange = range.upper - range.lower;
        if (this->property("isLogarithmic").toBool()){
            if (maxRange < 1e2){
               customPlot->yAxis->setRange(1e-2, range.lower + 1e2);//0.01~1000
            }
        } else if (maxRange < 1e2){
            customPlot->yAxis->setRange(range.lower, range.lower + 1e2);//0.01~1000
        } else if (range.lower < Y_AXIS_LOWER){
            customPlot->yAxis->setRange(Y_AXIS_LOWER, maxRange + 1e2);//0.01~1000
        }
    });
}

void PlotWidget::init()
{
    QWidget *dockWidgetContents = new QWidget();
    dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
    QGridLayout *gridLayout = new QGridLayout(dockWidgetContents);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    gridLayout->setContentsMargins(0, 0, 0, 0);

    customPlot = new QCustomPlot(this);
    //customPlot->setOpenGl(true); //不能启用硬件加速，否则多个控件数据刷新起冲突
    customPlot->installEventFilter(this);
//    customPlot->plotLayout()->insertRow(0);
//    customPlot->plotLayout()->addElement(0, 0, new QCPTextElement(customPlot, title, QFont("微软雅黑", 10, QFont::Bold)));

    gridLayout->addWidget(customPlot, 0, 0, 1, 1);
    //this->setWidget(dockWidgetContents);

    // 设置选择容忍度，即鼠标点击点到数据点的距离
    //customPlot->setSelectionTolerance(5);
    // 设置全局抗锯齿
    //customPlot->setAntialiasedElements(QCP::aeAll);
    customPlot->setNotAntialiasedElements(QCP::aeAll);
    // 图例名称隐藏
    customPlot->legend->setVisible(false);
    customPlot->legend->setFillOrder(QCPLayoutGrid::foColumnsFirst);//设置图例在一行中显示
    // 图例名称显示位置
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignRight);
    // 设置边界
    customPlot->setContentsMargins(0, 0, 0, 0);
    // 设置标签倾斜角度，避免显示不下
    customPlot->xAxis->setTickLabelRotation(-45);
    // 背景色
    customPlot->setBackground(QBrush(clrBackground));
    // 图像画布边界
    customPlot->axisRect()->setMinimumMargins(QMargins(0, 0, 0, 0));
    // 坐标背景色
    customPlot->axisRect()->setBackground(clrBackground);
    // 允许拖拽，缩放
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    // 允许轴自适应大小
    //customPlot->graph(0)->rescaleValueAxis(true);
    customPlot->xAxis->rescale(false);
    customPlot->yAxis->rescale(false);
    // 设置刻度范围
    //QSharedPointer<QCPAxisTickerFixed> axisTickerFixed(new QCPAxisTickerFixed);
    //axisTickerFixed->setTickStep(256);//每间隔256单位一个标签
    //axisTickerFixed->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);
    //axisTickerFixed->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssMeetTickCount);
    //customPlot->xAxis->setTicker(axisTickerFixed);
    customPlot->xAxis->setRange(0, 180);
    customPlot->yAxis->setRange(Y_AXIS_LOWER, 10000);
    customPlot->yAxis->ticker()->setTickCount(5);
    customPlot->xAxis->ticker()->setTickCount(16);
    //customPlot->xAxis->ticker()->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssMeetTickCount);
    //customPlot->yAxis2->setPadding(10);//距离右边的距离

//    QSharedPointer<QCPAxisTickerText> xAxisTickerText(new QCPAxisTickerText);
//    xAxisTickerText->setSubTickCount(10);
//    xAxisTickerText->setTickCount(4096);
//    customPlot->xAxis->setTicker(xAxisTickerText);

    // 设置刻度可见
    customPlot->xAxis->setTicks(axisVisible);
    customPlot->xAxis2->setTicks(false);
    customPlot->yAxis->setTicks(axisVisible);
    customPlot->yAxis2->setTicks(false);
    // 设置轴线可见
    customPlot->xAxis->setVisible(axisVisible);
    customPlot->xAxis2->setVisible(axisVisible);
    customPlot->yAxis->setVisible(axisVisible);
    customPlot->yAxis2->setVisible(axisVisible);
    //customPlot->axisRect()->setupFullAxesBox();//四边安装轴并显示
    // 设置刻度标签可见
    customPlot->xAxis->setTickLabels(axisVisible);
    customPlot->xAxis2->setTickLabels(false);
    customPlot->yAxis->setTickLabels(axisVisible);
    customPlot->yAxis2->setTickLabels(false);
    // 设置子刻度可见
    customPlot->xAxis->setSubTicks(axisVisible);
    customPlot->xAxis2->setSubTicks(false);
    customPlot->yAxis->setSubTicks(axisVisible);
    customPlot->yAxis2->setSubTicks(false);
    //设置轴标签名称
    //customPlot->xAxis->setLabel(QObject::tr("时间"));
    //customPlot->yAxis->setLabel(QObject::tr("能量/Kev"));
    // 设置网格线颜色
    customPlot->xAxis->grid()->setPen(QPen(QColor(180, 180, 180, 128), 1, Qt::PenStyle::DashLine));
    customPlot->yAxis->grid()->setPen(QPen(QColor(180, 180, 180, 128), 1, Qt::PenStyle::DashLine));
    customPlot->xAxis->grid()->setSubGridPen(QPen(QColor(50, 50, 50, 128), 1, Qt::DotLine));
    customPlot->yAxis->grid()->setSubGridPen(QPen(QColor(50, 50, 50, 128), 1, Qt::DotLine));
    customPlot->xAxis->grid()->setZeroLinePen(QPen(QColor(50, 50, 50, 100), 1, Qt::SolidLine));
    customPlot->yAxis->grid()->setZeroLinePen(QPen(QColor(50, 50, 50, 100), 1, Qt::SolidLine));
    // 设置网格线是否可见
    customPlot->xAxis->grid()->setVisible(false);
    customPlot->yAxis->grid()->setVisible(false);
    // 设置子网格线是否可见
    customPlot->xAxis->grid()->setSubGridVisible(false);
    customPlot->yAxis->grid()->setSubGridVisible(false);

    // 添加散点图
    QCPGraph * curGraph = customPlot->addGraph(customPlot->xAxis, customPlot->yAxis);
    curGraph->setAntialiased(false);
    curGraph->setPen(QPen(clrLine[0]));
    curGraph->selectionDecorator()->setPen(QPen(clrLine[0]));
    //curGraph->setLineStyle(QCPGraph::lsLine);
    curGraph->setLineStyle(QCPGraph::lsNone);// 隐藏线性图
    curGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 2));//显示散点图

    auto createStraightLineItem = [=](QCPItemStraightLine** _line){
        QCPItemStraightLine *line = new QCPItemStraightLine(customPlot);
        line->setLayer("overlay");
        line->setPen(QPen(Qt::gray, 1, Qt::DashLine));
        line->setClipToAxisRect(true);
        line->point1->setCoords(0, 0);
        line->point2->setCoords(0, 0);
        line->setVisible(false);
        *_line = line;
    };
    createStraightLineItem(&lineLeft);
    createStraightLineItem(&lineRight);

    // 文本元素随坐标变动而变动
    gaussResultItemText = new QCPItemText(customPlot);
    gaussResultItemText->setColor(QColor(0,128,128,255));// 字体色
    gaussResultItemText->setPen(QColor(130,130,130,255));// 边框
    gaussResultItemText->setBrush(QBrush(QColor(255,255,225,255)));// 背景色
    gaussResultItemText->setPositionAlignment(Qt::AlignBottom | Qt::AlignLeft);
    gaussResultItemText->position->setType(QCPItemPosition::ptPlotCoords);
    gaussResultItemText->setTextAlignment(Qt::AlignLeft);
    gaussResultItemText->setFont(QFont(font().family(), 12));
    gaussResultItemText->setPadding(QMargins(5, 5, 5, 5));
    gaussResultItemText->position->setCoords(250.0, 1000.0);//X、Y轴坐标值
    gaussResultItemText->setText("峰: 239.10 = 207.37 keV\n"
                            "半高宽: 40.15 FW[1/5]M:71.59\n"
                            "库: Sn-113[Tin]在255.04; 0 Bq\n"
                            "总计数面积: 623036\n"
                            "净面积: 57994 +/-3703\n"
                            "总计数/净计数 比率:731.18/68.06 cps");
    gaussResultItemText->setVisible(false);

    coordsTipItemText = new QCPItemText(customPlot);
    coordsTipItemText->setPositionAlignment(Qt::AlignBottom|Qt::AlignLeft);
    coordsTipItemText->position->setType(QCPItemPosition::ptAbsolute);
    coordsTipItemText->setFont(QFont(font().family(), 15));
    coordsTipItemText->setPen(QPen(Qt::black));
    coordsTipItemText->setBrush(Qt::white);
    coordsTipItemText->setVisible(false);

    flagItemLine = new QCPItemLine(customPlot);
    flagItemLine->setAntialiased(false);
    flagItemLine->start->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    flagItemLine->end->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    flagItemLine->setPen(QPen(QColor(255,0,255,255), 3, Qt::SolidLine));
    flagItemLine->setSelectedPen(QPen(QColor(255,0,255,255), 3, Qt::SolidLine));
    flagItemLine->setVisible(false);

    dragRectItem = new QCPItemRect(customPlot);
    dragRectItem->setAntialiased(false);
    dragRectItem->topLeft->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    dragRectItem->bottomRight->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    dragRectItem->setPen(QPen(clrSelect, 2, Qt::DotLine));// 边框
    dragRectItem->setBrush(Qt::NoBrush);// 背景色
    dragRectItem->setSelectedPen(QPen(clrSelect, 2, Qt::DotLine));
    dragRectItem->setVisible(false);

    // 图形刷新
    customPlot->replot();

//    QTimer *timerUpdate = new QTimer(this);
//    static int timerRef = 0;
//    static int lastV = qrand() % 100 + 5000;
//    connect(timerUpdate, &QTimer::timeout, this, [=](){
//        // 更新数据
//        lastV += qrand() % 100 - 50;
//        customPlot->graph(0)->addData(timerRef++, lastV);

//        // 保持图形范围合理
//        if (timerRef >= 2048)
//            customPlot->xAxis->setRange(timerRef - 2048, 2048, Qt::AlignLeft);

//        // 重新绘制
//        customPlot->replot();

//        customPlot->yAxis->rescale(true);
//    });
//    timerUpdate->start(10);

    connect(customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*, int, QMouseEvent*)), this, SLOT(slotPlotClick(QCPAbstractPlottable*, int, QMouseEvent*)));
    connect(customPlot, SIGNAL(beforeReplot()), this, SLOT(slotBeforeReplot()));
    connect(customPlot, SIGNAL(afterLayout()), this, SLOT(slotBeforeReplot()));

    connect(customPlot->xAxis, QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this, [=](const QCPRange &range){
        qint64 maxRange = range.upper - range.lower;
        if (range.lower < 0){
           customPlot->xAxis->setRange(0, maxRange);
        } else if (maxRange < 1e2){
            customPlot->xAxis->setRange(range.lower, range.lower + 1e2);//0.01~1000
         }
    });
    connect(customPlot->yAxis, QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this, [=](const QCPRange &range){
        qint64 maxRange = range.upper - range.lower;
        if (this->property("isLogarithmic").toBool()){
            if (maxRange < 1e2){
               customPlot->yAxis->setRange(1e-2, range.lower + 1e2);//0.01~1000
            }
        } else if (maxRange < 1e2){
            customPlot->yAxis->setRange(range.lower, range.lower + 1e2);//0.01~1000
        } else if (range.lower < Y_AXIS_LOWER){
            customPlot->yAxis->setRange(Y_AXIS_LOWER, maxRange + 1e2);//0.01~1000
        }
    });

    //connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));
    //connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));


//    QTimer::singleShot(50, this, [=](){
//        //随机数据
//        for (int index = 0; index < customPlot->graphCount(); ++index) {
//            int lastV = qrand() % 100 + 5000;
//            QVector<double> keys, values;
//            QVector<QColor> colors;
//            for (int i=0; i<1024; i++){
//                keys << i;

//                lastV += qrand() % 100 - 50;
//                values << lastV;

//                colors << clrLine[index];
//            }

//            customPlot->graph(index)->setData(keys, values, colors);
//        }

//        customPlot->replot();
//    });
}

void PlotWidget::initMultiCustomPlot()
{
    QWidget *dockWidgetContents = new QWidget();
    dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
    QGridLayout *gridLayout = new QGridLayout(dockWidgetContents);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    gridLayout->setContentsMargins(0, 0, 0, 0);

    customPlot = new QCustomPlot(this);
    //customPlot->setOpenGl(true); //不能启用硬件加速，否则多个控件数据刷新起冲突
    customPlot->installEventFilter(this);
//    customPlot->plotLayout()->insertRow(0);
//    customPlot->plotLayout()->addElement(0, 0, new QCPTextElement(customPlot, title, QFont("微软雅黑", 10, QFont::Bold)));

    gridLayout->addWidget(customPlot, 0, 0, 1, 1);
    //this->setWidget(dockWidgetContents);

    // 设置选择容忍度，即鼠标点击点到数据点的距离
    //customPlot->setSelectionTolerance(5);
    // 设置全局抗锯齿
    //customPlot->setAntialiasedElements(QCP::aeAll);
    customPlot->setNotAntialiasedElements(QCP::aeAll);
    // 图例名称显示
    customPlot->legend->setVisible(false);
    // 图例名称显示位置
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignRight);
    // 设置边界
    customPlot->setContentsMargins(0, 0, 0, 0);
    // 设置标签倾斜角度，避免显示不下
    customPlot->xAxis->setTickLabelRotation(-45);
    // 背景色
    customPlot->setBackground(QBrush(clrBackground));
    // 图像画布边界
    customPlot->axisRect()->setMinimumMargins(QMargins(0, 0, 0, 0));
    // 坐标背景色
    customPlot->axisRect()->setBackground(clrBackground);
    // 允许拖拽，缩放
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    // 允许轴自适应大小
    //customPlot->graph(0)->rescaleValueAxis(true);
    customPlot->xAxis->rescale(false);
    customPlot->yAxis->rescale(false);
    // 设置刻度范围    
//    QSharedPointer<QCPAxisTickerFixed> axisTickerFixed(new QCPAxisTickerFixed);
//    axisTickerFixed->setTickStep(256);//每间隔256单位一个标签
//    axisTickerFixed->setScaleStrategy(QCPAxisTickerFixed::ssPowers);
//    axisTickerFixed->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssReadability);
//    customPlot->xAxis->setTicker(axisTickerFixed);
    QSharedPointer<QCPAxisTickerLog> axisTickerFixed(new QCPAxisTickerLog);
    //axisTickerFixed->setLogBase(10);
    //axisTickerFixed->setSubTickCount(5);
    customPlot->yAxis->setTicker(axisTickerFixed);
    customPlot->yAxis->setRange(Y_AXIS_LOWER, 10000);
    //customPlot->yAxis->ticker()->setTickCount(5);

    customPlot->xAxis->setRange(0, 4096);
    customPlot->xAxis->ticker()->setTickCount(16);
    //customPlot->xAxis->ticker()->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssReadability);
    //customPlot->yAxis2->setPadding(10);//距离右边的距离

//    QSharedPointer<QCPAxisTickerText> xAxisTickerText(new QCPAxisTickerText);
//    xAxisTickerText->setSubTickCount(10);
//    xAxisTickerText->setTickCount(4096);
//    customPlot->xAxis->setTicker(xAxisTickerText);

    // 设置刻度可见    
    customPlot->xAxis->setTicks(axisVisible);
    customPlot->xAxis2->setTicks(false);
    customPlot->yAxis->setTicks(axisVisible);
    customPlot->yAxis2->setTicks(false);
    // 设置轴线可见
    customPlot->xAxis->setVisible(axisVisible);
    customPlot->xAxis2->setVisible(axisVisible);
    customPlot->yAxis->setVisible(axisVisible);
    customPlot->yAxis2->setVisible(axisVisible);
    //customPlot->axisRect()->setupFullAxesBox();//四边安装轴并显示
    // 设置刻度标签可见
    customPlot->xAxis->setTickLabels(axisVisible);
    customPlot->xAxis2->setTickLabels(false);
    customPlot->yAxis->setTickLabels(axisVisible);
    customPlot->yAxis2->setTickLabels(false);
    // 设置子刻度可见
    customPlot->xAxis->setSubTicks(axisVisible);
    customPlot->xAxis2->setSubTicks(false);
    customPlot->yAxis->setSubTicks(axisVisible);
    customPlot->yAxis2->setSubTicks(false);
    //设置轴标签名称
    //customPlot->xAxis->setLabel(QObject::tr("计数"));
    //customPlot->yAxis->setLabel(QObject::tr("能量/Kev"));
    // 设置网格线颜色
    customPlot->xAxis->grid()->setPen(QPen(QColor(180, 180, 180, 128), 1, Qt::PenStyle::DashLine));
    customPlot->yAxis->grid()->setPen(QPen(QColor(180, 180, 180, 128), 1, Qt::PenStyle::DashLine));
    customPlot->xAxis->grid()->setSubGridPen(QPen(QColor(50, 50, 50, 128), 1, Qt::DotLine));
    customPlot->yAxis->grid()->setSubGridPen(QPen(QColor(50, 50, 50, 128), 1, Qt::DotLine));
    customPlot->xAxis->grid()->setZeroLinePen(QPen(QColor(50, 50, 50, 100), 1, Qt::SolidLine));
    customPlot->yAxis->grid()->setZeroLinePen(QPen(QColor(50, 50, 50, 100), 1, Qt::SolidLine));
    // 设置网格线是否可见
    customPlot->xAxis->grid()->setVisible(false);
    customPlot->yAxis->grid()->setVisible(false);
    // 设置子网格线是否可见
    customPlot->xAxis->grid()->setSubGridVisible(false);
    customPlot->yAxis->grid()->setSubGridVisible(false);

    // 添加能谱图
    for (int i=0; i<2; ++i){
        QCPGraph * curGraph = customPlot->addGraph(customPlot->xAxis, customPlot->yAxis);
        if (0 == i)
            curGraph->setName(tr("能谱"));
        else
            curGraph->setName(tr("计数"));
        curGraph->setAntialiased(false);
        curGraph->setPen(QPen(clrLine[i]));
        curGraph->selectionDecorator()->setPen(QPen(clrLine[i]));
        //curGraph->setLineStyle(QCPGraph::lsLine);
        curGraph->setLineStyle(QCPGraph::lsNone);// 隐藏线性图
        curGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 2));//显示散点图
    }

    // 添加高斯曲线图
    QCPGraph *graphCurve = customPlot->addGraph(customPlot->xAxis, customPlot->yAxis);
    graphCurve->setName(QString("高斯曲线"));
    graphCurve->setAntialiased(true);
    graphCurve->setPen(QPen(clrGauseCurve));
    graphCurve->selectionDecorator()->setPen(QPen(clrGauseCurve));
    graphCurve->setLineStyle(QCPGraph::lsLine);
    graphCurve->setVisible(false);

    auto createStraightLineItem = [=](QCPItemStraightLine** _line){
        QCPItemStraightLine *line = new QCPItemStraightLine(customPlot);
        line->setLayer("overlay");
        line->setPen(QPen(Qt::gray, 1, Qt::DashLine));
        line->setClipToAxisRect(true);
        line->point1->setCoords(0, 0);
        line->point2->setCoords(0, 0);
        line->setVisible(false);
        *_line = line;
    };
    createStraightLineItem(&lineLeft);
    createStraightLineItem(&lineRight);

    // 文本元素随坐标变动而变动
    gaussResultItemText = new QCPItemText(customPlot);
    gaussResultItemText->setColor(QColor(0,128,128,255));// 字体色
    gaussResultItemText->setPen(QColor(130,130,130,255));// 边框
    gaussResultItemText->setBrush(QBrush(QColor(255,255,225,255)));// 背景色
    gaussResultItemText->setPositionAlignment(Qt::AlignBottom | Qt::AlignLeft);
    gaussResultItemText->position->setType(QCPItemPosition::ptPlotCoords);
    gaussResultItemText->setTextAlignment(Qt::AlignLeft);
    gaussResultItemText->setFont(QFont(font().family(), 12));
    gaussResultItemText->setPadding(QMargins(5, 5, 5, 5));
    gaussResultItemText->position->setCoords(250.0, 1000.0);//X、Y轴坐标值
    gaussResultItemText->setText("峰: 239.10 = 207.37 keV\n"
                            "半高宽: 40.15 FW[1/5]M:71.59\n"
                            "库: Sn-113[Tin]在255.04; 0 Bq\n"
                            "总计数面积: 623036\n"
                            "净面积: 57994 +/-3703\n"
                            "总计数/净计数 比率:731.18/68.06 cps");
    gaussResultItemText->setVisible(false);

    coordsTipItemText = new QCPItemText(customPlot);
    coordsTipItemText->setPositionAlignment(Qt::AlignBottom|Qt::AlignLeft);
    coordsTipItemText->position->setType(QCPItemPosition::ptAbsolute);
    coordsTipItemText->setFont(QFont(font().family(), 15));
    coordsTipItemText->setPen(QPen(Qt::black));
    coordsTipItemText->setBrush(Qt::white);
    coordsTipItemText->setVisible(false);

    flagItemLine = new QCPItemLine(customPlot);
    flagItemLine->setAntialiased(false);
    flagItemLine->start->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    flagItemLine->end->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    flagItemLine->setPen(QPen(QColor(255,0,255,255), 3, Qt::SolidLine));
    flagItemLine->setSelectedPen(QPen(QColor(255,0,255,255), 3, Qt::SolidLine));
    flagItemLine->setVisible(false);

    dragRectItem = new QCPItemRect(customPlot);
    dragRectItem->setAntialiased(false);
    dragRectItem->topLeft->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    dragRectItem->bottomRight->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    dragRectItem->setPen(QPen(clrSelect, 2, Qt::DotLine));// 边框
    dragRectItem->setBrush(Qt::NoBrush);// 背景色
    dragRectItem->setSelectedPen(QPen(clrSelect, 2, Qt::DotLine));
    dragRectItem->setVisible(false);

    connect(customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*, int, QMouseEvent*)), this, SLOT(slotPlotClick(QCPAbstractPlottable*, int, QMouseEvent*)));
    connect(customPlot, SIGNAL(beforeReplot()), this, SLOT(slotBeforeReplot()));
    connect(customPlot, SIGNAL(afterLayout()), this, SLOT(slotBeforeReplot()));

    connect(customPlot->xAxis, QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this, [=](const QCPRange &range){
        qint64 maxRange = range.upper - range.lower;
        if (range.lower < 0){
           customPlot->xAxis->setRange(0, maxRange);
        } else if (maxRange < 1e2){
            customPlot->xAxis->setRange(range.lower, range.lower + 1e2);//0.01~1000
         }
    });
    connect(customPlot->yAxis, QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this, [=](const QCPRange &range){
        qint64 maxRange = range.upper - range.lower;
        if (this->property("isLogarithmic").toBool()){
            if (maxRange < 1e2){
               customPlot->yAxis->setRange(1e-2, range.lower + 1e2);//0.01~1000
            }
        } else if (maxRange < 1e2){
            customPlot->yAxis->setRange(range.lower, range.lower + 1e2);//0.01~1000
        } else if (range.lower < Y_AXIS_LOWER){
            customPlot->yAxis->setRange(Y_AXIS_LOWER, maxRange + 1e2);//0.01~1000
        }
    });

    switchShowModel(false);
    switchDataModel(false);

    // 图形刷新
    customPlot->replot();

//    QTimer::singleShot(50, this, [=](){
//        //随机数据
//        for (int index = 0; index < customPlot->graphCount(); ++index) {
//            int lastV = qrand() % 100 + 5000;
//            QVector<double> keys, values;
//            QVector<QColor> colors;
//            for (int i=0; i<1024; i++){
//                keys << i;

//                lastV += qrand() % 100 - 50;
//                values << lastV;

//                colors << clrLine[index];
//            }

//            customPlot->graph(index)->setData(keys, values, colors);
//        }

//        customPlot->replot();
//    });
}

#include <QMouseEvent>
bool PlotWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != this){
        if (event->type() == QEvent::MouseButtonPress){
            QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
            if (watched->inherits("QCustomPlot")){
                QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(watched);
                if (e->button() == Qt::LeftButton) {
                    if (e->modifiers() & Qt::ControlModifier){//Ctrl键按下，禁止拖拽功能
                        customPlot->setInteraction(QCP::iRangeDrag, false);
                    }

                    isPressed = true;
                    dragStart = e->pos();
                } else if (e->button() == Qt::RightButton) {// 右键恢复
                    QMutexLocker locker(&mutexRefreshPlot);

                    isPressed = false;
                    isDragging = false;
                    double maxEnergy = 0;
                    double minKey = 0;

                    // 先恢复数据颜色
                    QCPGraph *graph = customPlot->graph(0);
                    QVector<double> keys, values;
                    QVector<QColor> colors;
                    int count = graph->data()->size();
                    if (count > 0)
                        minKey = (double)graph->data()->at(0)->key;

                    for (int i=0; i<count; ++i){
                        keys << (double)graph->data()->at(i)->key;
                        values << (double)graph->data()->at(i)->value;
                        colors << clrLine;

                        maxEnergy = qMax(maxEnergy, values[i]);
                        minKey = qMin(minKey, keys[i]);
                    }
                    customPlot->graph(0)->setData(keys, values, colors);


                    QCPItemRect* dragRectItem = customPlot->findChild<QCPItemRect*>("dragRectItem");//鼠标拖拽框
                    if (dragRectItem)
                        dragRectItem->setVisible(false);
                    QCPItemText* gaussResultItemText = customPlot->findChild<QCPItemText*>("gaussResultItemText");//拟合信息
                    if (gaussResultItemText)
                        gaussResultItemText->setVisible(false);
                    QCPItemLine* flagItemLine = customPlot->findChild<QCPItemLine*>("flagItemLine");//点选标记竖线
                    if (flagItemLine)
                        flagItemLine->setVisible(false);

                    if (this->property("isRefModel").toBool()){
                        customPlot->xAxis->setRange(minKey, minKey + 180);//计数模式
                        customPlot->yAxis->setRange(Y_AXIS_LOWER, maxEnergy / RANGE_SCARRE_LOWER);
                    }
                    else{
                        customPlot->xAxis->setRange(minKey, minKey + 4096);//能谱模式
                        customPlot->yAxis->setRange(Y_AXIS_LOWER, maxEnergy / RANGE_SCARRE_LOWER);
                    }

                    customPlot->replot();
                    setCursor(Qt::ArrowCursor);
                }
            }
        } else if (event->type() == QEvent::MouseMove){
            QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
            if (watched->inherits("QCustomPlot")){
                QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(watched);
                if (isPressed){
                    if (!isDragging) {
                        if (e->modifiers() & Qt::ControlModifier){//鼠标框选开始
                            QMutexLocker locker(&mutexRefreshPlot);
                            QCPGraph *graph = customPlot->graph(0);

                            QCPItemText* gaussResultItemText = customPlot->findChild<QCPItemText*>("gaussResultItemText");
                            if (gaussResultItemText)
                                gaussResultItemText->setVisible(false);

                            QCPItemRect* dragRectItem = customPlot->findChild<QCPItemRect*>("dragRectItem");
                            if (dragRectItem){
                                dragRectItem->setProperty("top", dragStart.y());
                                dragRectItem->setProperty("left", dragStart.x());

                                double key, value;
                                graph->pixelsToCoords(dragStart.x(), dragStart.y(), key, value);
                                dragRectItem->topLeft->setCoords(key, value);

                                dragRectItem->setVisible(false);
                            }

                            customPlot->replot();
                            setCursor(Qt::CrossCursor);
                        } else {
                            setCursor(Qt::SizeAllCursor);
                        }

                        isDragging = true;
                    }else {
                        if (e->modifiers() & Qt::ControlModifier && customPlot->graph(ENGRY_GRAPH)->visible()){
                            QMutexLocker locker(&mutexRefreshPlot);

                            QCPItemRect* dragRectItem = customPlot->findChild<QCPItemRect*>("dragRectItem");
                            if (dragRectItem){
                                dragRectItem->setProperty("bottom", e->pos().y());
                                dragRectItem->setProperty("right", e->pos().x());

                                double key, value;
                                customPlot->graph(0)->pixelsToCoords(e->pos().x(), e->pos().y(), key, value);
                                dragRectItem->bottomRight->setCoords(key, value);
                                dragRectItem->setVisible(true);

                                customPlot->replot();
                            }
                        }
                    }
                }                
            }
        } else if (event->type() == QEvent::MouseButtonRelease){
            QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
            if (watched->inherits("QCustomPlot")){
                QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(watched);
                if (e->button() == Qt::LeftButton) { //松开鼠标左键
                    setCursor(Qt::ArrowCursor);

                    if (e->modifiers() & Qt::ControlModifier){//Ctrl按键
                        QCPItemRect* dragRectItem = customPlot->findChild<QCPItemRect*>("dragRectItem");
                        if (dragRectItem && dragRectItem->visible()){
                            QMutexLocker locker(&mutexRefreshPlot);

                            QCPGraph *graph = customPlot->graph(0);
                            int fcount = 0;
                            std::vector<double> sx;
                            std::vector<double> sy;
                            //框选完毕，将选中的点颜色更新
                            {
                                double key_from = dragRectItem->topLeft->key();
                                double key_to = dragRectItem->bottomRight->key();
                                double key_temp = key_from;
                                key_from = qMin(key_from, key_to);
                                key_to = qMax(key_temp, key_to);

                                double value_from = dragRectItem->topLeft->value();
                                double value_to = dragRectItem->bottomRight->value();
                                double value_temp = value_from;
                                value_from = qMax(value_from, value_to);
                                value_to = qMin(value_temp, value_to);

                                QVector<double> keys, values, curveKeys;
                                QVector<QColor> colors;
                                bool inArea = false;
                                for (int i=0; i<graph->data()->size(); ++i){
                                    if (graph->data()->at(i)->key>=key_from && graph->data()->at(i)->key<=key_to && graph->data()->at(i)->value>=value_to) {
                                        inArea = true;
                                        keys << (double)graph->data()->at(i)->key;
                                        values << (double)graph->data()->at(i)->value;
                                        colors << clrRang;

                                        fcount++;
                                        sx.push_back((double)graph->data()->at(i)->key);
                                        sy.push_back((double)graph->data()->at(i)->value);

                                        curveKeys << (double)graph->data()->at(i)->key;
                                    } else {
                                        keys << (double)graph->data()->at(i)->key;
                                        values << (double)graph->data()->at(i)->value;
                                        colors << graph->data()->at(i)->color;
                                    }
                                }

                                // 这里需要对数据去重和排序处理
                                graph->setData(keys, values, colors);
                                customPlot->replot();
                            }

                            // 高斯拟合
                            {
                                //显示拟合曲线
                                double result[3];
                                fit_GaussCurve(fcount, sx, sy, result);

                                //显示拟合数据
                                QCPItemText* gaussResultItemText = customPlot->findChild<QCPItemText*>("gaussResultItemText");
                                if (gaussResultItemText){
                                    QString info = QString("半高宽：%1\n峰: %2 = %3")
                                            .arg(QString::number(result[0], 'f', 3))
                                            .arg(QString::number(result[1], 'f', 0))
                                            .arg(QString::number(result[2], 'f', 3));

                                    gaussResultItemText->setText(info);
                                    double key, value;
                                    graph->pixelsToCoords(e->pos().x(), e->pos().y(), key, value);
                                    gaussResultItemText->position->setCoords(key, value);
                                    gaussResultItemText->setVisible(true);
                                }

                                //计算符合能窗
                                unsigned int minMean = (result[1] - result[0] / 2);
                                unsigned int maxMean = (result[1] + result[0] / 2);
                                this->setProperty("ActiveCustomPlotName", customPlot->objectName());
                                emit sigUpdateMeanValues(customPlot->objectName(), minMean, maxMean);
                            }
                        }
                    }

                    isPressed = false;
                    isDragging = false;

                    customPlot->setInteraction(QCP::iRangeDrag, true);
                }
            }
        } else if (event->type() == QEvent::MouseButtonDblClick){
            emit sigMouseDoubleClickEvent();
        }
    }

    return QWidget::eventFilter(watched, event);
}

#include <QTimer>
void PlotWidget::slotPlotClick(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event)
{
    QMutexLocker locker(&mutexRefreshPlot);
    QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(sender());
    QCPItemText* coordsTipItemText = customPlot->findChild<QCPItemText*>("coordsTipItemText");
    if (coordsTipItemText){
        QCPGraph *graph = qobject_cast<QCPGraph*>(plottable);//customPlot->graph(0);
        QSharedPointer<QCPGraphDataContainer> data = graph->data();
        const QCPGraphData *ghd = data->at(dataIndex);

        //显示轴信息
        QString text = QString::number(ghd->key,10,0) + "," + QString::number(ghd->value,10,0);
        coordsTipItemText->setText(text);
        coordsTipItemText->position->setCoords(event->pos().x() + 3, event->pos().y() - 3);
        coordsTipItemText->setVisible(true);

        //显示标记点
        QCPItemLine* flagItemLine = customPlot->findChild<QCPItemLine*>("flagItemLine");
        if (flagItemLine){
            flagItemLine->setProperty("key", ghd->key);
            flagItemLine->setProperty("value", ghd->value);
            flagItemLine->setVisible(true);
        }

        customPlot->replot();
    }


    setCursor(Qt::ArrowCursor);
    QTimer::singleShot(1000, this, [=](){
       coordsTipItemText->setVisible(false);
       customPlot->replot();
    });
}

void PlotWidget::slotBeforeReplot()
{
    QMutexLocker locker(&mutexRefreshPlot);
    QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(sender());
    QCPItemLine* flagItemLine = customPlot->findChild<QCPItemLine*>("flagItemLine");
    if (flagItemLine && flagItemLine->visible()){
        QCPGraph *graph = customPlot->graph(0);
        double key = flagItemLine->property("key").toUInt();
        double value = flagItemLine->property("value").toUInt();
        QPointF pixels = graph->coordsToPixels(key, value);
        graph->pixelsToCoords(pixels.x() + 1, pixels.y() - 2, key, value);
        flagItemLine->start->setCoords(key, value);
        graph->pixelsToCoords(pixels.x() + 1, pixels.y() - 32, key, value);
        flagItemLine->end->setCoords(key, value);
        customPlot->replot();
    }
}

#include <QtMath>
#include <math.h>
void PlotWidget::slotUpdatePlotDatas(vector<SingleSpectrum> r1, vector<CurrentPoint> r2, vector<CoincidenceResult> r3)
{
    QMutexLocker locker(&mutexRefreshPlot);
    QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
    QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
    QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

    if (this->property("isRefModel").toBool()){//计数模式
        {//Det1
            double maxPoint = 0.;
            QVector<double> keys, values;
            QVector<QColor> colors;
            for (size_t i=0; i<r2.size(); ++i){
                keys << i+1;
                values << r2[i].dataPoint1;
                colors << clrLine;
                maxPoint = qMax(maxPoint, values[i]);
            }

            customPlotDet1->graph(0)->setData(keys, values, colors);
            if (maxPoint > customPlotDet1->yAxis->range().upper * RANGE_SCARRE_UPPER){
                customPlotDet1->yAxis->setRange(customPlotDet1->yAxis->range().lower, maxPoint / RANGE_SCARRE_LOWER);
            }
            customPlotDet1->replot();
        }

        {//Det2
            double maxPoint = 0.;
            QVector<double> keys, values;
            QVector<QColor> colors;
            for (size_t i=0; i<r2.size(); ++i){
                keys << i+1;
                values << r2[i].dataPoint2;
                colors << clrLine;
                maxPoint = qMax(maxPoint, values[i]);
            }

            if (customPlotDet2->graph(0)->visible()){
                customPlotDet2->graph(0)->setData(keys, values, colors);
                if (maxPoint > customPlotDet2->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotDet2->yAxis->setRange(customPlotDet2->yAxis->range().lower, maxPoint / RANGE_SCARRE_LOWER);
                }
            }
            customPlotDet2->replot();
        }

        {//符合结果
            QVector<double> keys, values;
            QVector<QColor> colors;
            double maxCount = 0;

            for (size_t i=0; i<r3.size(); ++i){
                keys << (i+1);
                values << r3[i].ConCount_single;
                colors << clrLine;
                maxCount = qMax(maxCount, values[i]);
            }

            if (maxCount > customPlotCoincidenceResult->yAxis->range().upper * RANGE_SCARRE_UPPER){
                customPlotCoincidenceResult->yAxis->setRange(customPlotCoincidenceResult->yAxis->range().lower, maxCount / RANGE_SCARRE_LOWER);
            }
            customPlotCoincidenceResult->graph(0)->setData(keys, values, colors);
            customPlotCoincidenceResult->replot();
        }
    } else {//能谱
        {//Det1
            double maxEnergy = 0;
            QVector<double> keys, values;
            QVector<QColor> colors;

            for (size_t j=0; j<r1.size(); ++j){
                for (int i=0; i<MULTI_CHANNEL; ++i){
                    energyValues[0][i] += r1[j].spectrum[0][i];
                }
            }

            for (int i=0; i<MULTI_CHANNEL; ++i){
                keys << i+1;
                colors << clrLine;
                values << energyValues[0][i];
                maxEnergy = qMax(maxEnergy, values[i]);
            }

            customPlotDet1->graph(0)->setData(keys, values, colors);
            if (maxEnergy > customPlotDet1->yAxis->range().upper * RANGE_SCARRE_UPPER){
                customPlotDet1->yAxis->setRange(customPlotDet1->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
            }

            customPlotDet1->replot();
        }

        {//Det2
            double maxEnergy = 0;
            QVector<double> keys, values;
            QVector<QColor> colors;

            for (size_t j=0; j<r1.size(); ++j){
                for (int i=0; i<MULTI_CHANNEL; ++i){
                    energyValues[1][i] += r1[j].spectrum[0][i];
                }
            }

            for (int i=0; i<MULTI_CHANNEL; ++i){
                keys << i+1;
                colors << clrLine;
                values << energyValues[1][i];
                maxEnergy = qMax(maxEnergy, values[i]);
            }

            customPlotDet1->graph(0)->setData(keys, values, colors);
            if (maxEnergy > customPlotDet1->yAxis->range().upper * RANGE_SCARRE_UPPER){
                customPlotDet1->yAxis->setRange(customPlotDet1->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
            }

            customPlotDet1->replot();
        }
    }
}

void PlotWidget::slotResetPlot()
{
    QMutexLocker locker(&mutexRefreshPlot);
    QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
    QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
    QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

    customPlotDet1->graph(0)->data()->clear();
    customPlotDet2->graph(0)->data()->clear();
    customPlotCoincidenceResult->graph(0)->data()->clear();

    QVector<double>(MULTI_CHANNEL, 0).swap(energyValues[0]);
    QVector<double>(MULTI_CHANNEL, 0).swap(energyValues[1]);
    customPlotDet1->replot();
    customPlotDet2->replot();
    customPlotCoincidenceResult->replot();
}

void PlotWidget::switchShowModel(bool refModel)
{
    QMutexLocker locker(&mutexRefreshPlot);
    this->setProperty("isRefModel", refModel);
    QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
    QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
    QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

    customPlotDet1->graph(0)->data()->clear();
    customPlotDet2->graph(0)->data()->clear();
    customPlotCoincidenceResult->graph(0)->data()->clear();

    if (refModel){
        customPlotDet1->xAxis->setRange(0, 180); // 设置x轴的显示范围
    } else {
        customPlotDet1->xAxis->setRange(0, MULTI_CHANNEL); // 设置x轴的显示范围
    }

    customPlotDet1->replot();
}

void PlotWidget::switchDataModel(bool isLogarithmic)
{
    QMutexLocker locker(&mutexRefreshPlot);
    this->setProperty("isLogarithmic", isLogarithmic);
    QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
    QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
    QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");
    /*
        对于参数formatCode可以参考QT的Qstring::number
        f 普通数字格式，范围过大时出现刻度重叠问题。默认保留小数点后六位，使用void QCPAxis::setNumberPrecision ( int precision)来控制保留多少位的小数点
        g 较小的数采用普通数字格式，较大的数采用科学计数（形如 5e6）.使用void QCPAxis::setNumberPrecision ( int precision)来控制多大的数字后采用科学计数。
        b qcustomplot独有的格式beautiful。和其他连用 如gb，较小的数采用普通数字格式，较大的数采用科学计数(形如 )。使用void QCPAxis::setNumberPrecision ( int precision)。来控制多大的数字后面采用科学计数
        c qcustomplot独有的格式将点符号乘号修改成x符号乘号
    */
    if (isLogarithmic){
        QCustomPlot* customPlots[3] = {customPlotDet1, customPlotDet2, customPlotCoincidenceResult};
        for (auto customPlot : customPlots){
            customPlot->yAxis->setScaleType(QCPAxis::ScaleType::stLogarithmic);
            customPlot->yAxis->setNumberFormat("eb");//使用科学计数法表示刻度
            customPlot->yAxis->setNumberPrecision(0);//小数点后面小数位数

            QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
            customPlot->yAxis->setTicker(logTicker);
            //customPlot->xAxis->setRange(0, 180);
            customPlot->yAxis->setRange(1e-2, 1e5);

            customPlot->replot();
        }
    } else {
        QCustomPlot* customPlots[3] = {customPlotDet1, customPlotDet2, customPlotCoincidenceResult};
        for (auto customPlot : customPlots){
            QSharedPointer<QCPAxisTicker> ticker(new QCPAxisTicker);
            customPlot->yAxis->setTicker(ticker);

            customPlot->yAxis->setScaleType(QCPAxis::ScaleType::stLinear);
            customPlot->yAxis->setRange(0, 10000);
            customPlot->yAxis->setNumberFormat("f");
            customPlot->yAxis->setNumberPrecision(0);

            customPlot->xAxis->setRange(0, 4096);
            customPlot->yAxis->setRange(Y_AXIS_LOWER, 1e4);

            customPlot->replot();
        }
    }    
}

void PlotWidget::slotGauss(int leftE, int rightE)
{
    QMutexLocker locker(&mutexRefreshPlot);
    QCustomPlot* customPlot = nullptr;
    if (this->property("ActiveCustomPlotName") == "Det1"){
        customPlot = this->findChild<QCustomPlot*>("Det1");
    } else {
        customPlot = this->findChild<QCustomPlot*>("Det2");
    }

    QCPItemRect* dragRectItem = customPlot->findChild<QCPItemRect*>("dragRectItem");//鼠标拖拽框
    if (dragRectItem){
        double key_from = dragRectItem->topLeft->key();
        double key_to = dragRectItem->bottomRight->key();
        double key_temp = key_from;
        key_from = qMin(key_from, key_to);
        key_to = qMax(key_temp, key_to);

        double value_from = dragRectItem->topLeft->value();
        double value_to = dragRectItem->bottomRight->value();
        double value_temp = value_from;
        value_from = qMin(value_from, value_to);
        value_to = qMax(value_temp, value_to);

        QCPGraph *graph = customPlot->graph(currentGraphIndex);
        QVector<double> keys, values, curveKeys;
        QVector<QColor> colors;
        int fcount = 0;
        std::vector<double> sx;
        std::vector<double> sy;
        for (int i=0; i<graph->data()->size(); ++i){
            if (graph->data()->at(i)->key>=leftE && graph->data()->at(i)->key<=rightE) {
                keys << (double)graph->data()->at(i)->key;
                values << (double)graph->data()->at(i)->value;
                colors << clrRang;

                fcount++;
                sx.push_back((double)graph->data()->at(i)->key);
                sy.push_back((double)graph->data()->at(i)->value);

                curveKeys << (double)graph->data()->at(i)->key;
            } else {
                keys << (double)graph->data()->at(i)->key;
                values << (double)graph->data()->at(i)->value;
                colors << graph->data()->at(i)->color;
            }
        }

        // 高斯拟合
        {
            double result[3];
            fit_GaussCurve(fcount, sx, sy, result);

            //绘制拟合曲线
            QCPGraph *graphCurve = customPlot->graph(1);
            QVector<double> curveValues;
            double a = result[2];
            double u = result[1];
            double FWHM = result[0];
            double ln2 = log(2);
            for (int i=0; i<curveKeys.size(); ++i){
                //a*exp[-4ln2(x-u)^2/FWHM^2]，a=result[2],u=result[1],FWHM=result[0].
                double x = curveKeys[i];
                double v = a*exp(-4*ln2*pow(x-u,2)/pow(FWHM, 2));

                curveValues.push_back(v);
            }

            graphCurve->setData(curveKeys, curveValues);
            graphCurve->setVisible(true);
            customPlot->replot();
        }
    }
}
