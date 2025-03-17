#include "plotwidget.h"
#include "qcustomplot.h"
#include <QGridLayout>
#include <QtMath>
#include <cmath>

PlotWidget::PlotWidget(QWidget *parent) : QDockWidget(parent)
    , titleTextTtem(nullptr)
    , coordsTextItem(nullptr)
    , textTipItem(nullptr)
    , lineFlagItem(nullptr)
{
    this->setContentsMargins(0, 0, 0, 0);
    this->setAllowedAreas(Qt::AllDockWidgetAreas);
    this->setFeatures(QDockWidget::DockWidgetMovable|DockWidgetFloatable/*QDockWidget::AllDockWidgetFeatures*/);
}

void PlotWidget::initCustomPlot()
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
    this->setWidget(dockWidgetContents);

    // 设置选择容忍度，即鼠标点击点到数据点的距离
    //customPlot->setSelectionTolerance(5);
    // 设置全局抗锯齿
    //customPlot->setAntialiasedElements(QCP::aeAll);
    customPlot->setNotAntialiasedElements(QCP::aeAll);
    // 图例名称隐藏
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
    QSharedPointer<QCPAxisTickerFixed> axisTickerFixed(new QCPAxisTickerFixed);
    axisTickerFixed->setTickStep(256);//每间隔256单位一个标签
    axisTickerFixed->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);
    //axisTickerFixed->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssMeetTickCount);
    customPlot->xAxis->setTicker(axisTickerFixed);
    customPlot->xAxis->setRange(0, 4096);
    customPlot->yAxis->setRange(0, 10000);
    customPlot->yAxis->ticker()->setTickCount(5);
    customPlot->xAxis->ticker()->setTickCount(16);
    //customPlot->xAxis->ticker()->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssMeetTickCount);
    //customPlot->yAxis2->setPadding(10);//距离右边的距离

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
    customPlot->xAxis->setSubTicks(false);
    customPlot->xAxis2->setSubTicks(false);
    customPlot->yAxis->setSubTicks(false);
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

    // 添加散点图
    QCPGraph * curGraph = customPlot->addGraph(customPlot->xAxis, customPlot->yAxis);
    curGraph->setAntialiased(false);
    curGraph->setPen(QPen(clrLine[0]));
    curGraph->selectionDecorator()->setPen(QPen(clrLine[0]));
    //curGraph->setLineStyle(QCPGraph::lsLine);
    curGraph->setLineStyle(QCPGraph::lsNone);// 隐藏线性图
    curGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 2));//显示散点图

    // 文本元素随窗口变动而变动
//    titleTextTtem = new QCPItemText(customPlot);
//    titleTextTtem->setColor(clrLine);
//    titleTextTtem->position->setType(QCPItemPosition::ptAbsolute);
//    titleTextTtem->setPositionAlignment(Qt::AlignTop | Qt::AlignLeft);
//    titleTextTtem->setTextAlignment(Qt::AlignLeft);
//    titleTextTtem->setFont(QFont(font().family(), 12));
//    titleTextTtem->setPadding(QMargins(8, 0, 0, 0));
//    titleTextTtem->position->setCoords(10.0, 10.0);//窗口坐标值
//    titleTextTtem->setText(QString("%1").arg(title));
//    titleTextTtem->setVisible(false);

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
    coordsTextItem = new QCPItemText(customPlot);
    coordsTextItem->setColor(QColor(0,128,128,255));// 字体色
    coordsTextItem->setPen(QColor(130,130,130,255));// 边框
    coordsTextItem->setBrush(QBrush(QColor(255,255,225,255)));// 背景色
    coordsTextItem->setPositionAlignment(Qt::AlignBottom | Qt::AlignLeft);
    coordsTextItem->position->setType(QCPItemPosition::ptPlotCoords);
    coordsTextItem->setTextAlignment(Qt::AlignLeft);
    coordsTextItem->setFont(QFont(font().family(), 12));
    coordsTextItem->setPadding(QMargins(5, 5, 5, 5));
    coordsTextItem->position->setCoords(250.0, 1000.0);//X、Y轴坐标值
    coordsTextItem->setText("峰: 239.10 = 207.37 keV\n"
                            "半高宽: 40.15 FW[1/5]M:71.59\n"
                            "库: Sn-113[Tin]在255.04; 0 Bq\n"
                            "总计数面积: 623036\n"
                            "净面积: 57994 +/-3703\n"
                            "总计数/净计数 比率:731.18/68.06 cps");
    coordsTextItem->setVisible(false);

    textTipItem = new QCPItemText(customPlot);
    textTipItem->setPositionAlignment(Qt::AlignBottom|Qt::AlignLeft);
    textTipItem->position->setType(QCPItemPosition::ptAbsolute);
    textTipItem->setFont(QFont(font().family(), 15));
    textTipItem->setPen(QPen(Qt::black));
    textTipItem->setBrush(Qt::white);
    textTipItem->setVisible(false);

    lineFlagItem = new QCPItemLine(customPlot);
    lineFlagItem->setAntialiased(false);
    lineFlagItem->start->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    lineFlagItem->end->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    lineFlagItem->setPen(QPen(QColor(255,0,255,255), 3, Qt::SolidLine));
    lineFlagItem->setSelectedPen(QPen(QColor(255,0,255,255), 3, Qt::SolidLine));
    lineFlagItem->setVisible(false);

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
    //connect(customPlot, SIGNAL(selectionChangedByUser()), this, SLOT(slotSelectionChanged()));
    connect(customPlot, SIGNAL(beforeReplot()), this, SLOT(slotBeforeReplot()));
    connect(customPlot, SIGNAL(afterLayout()), this, SLOT(slotBeforeReplot()));

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
    this->setWidget(dockWidgetContents);

    // 设置选择容忍度，即鼠标点击点到数据点的距离
    //customPlot->setSelectionTolerance(5);
    // 设置全局抗锯齿
    //customPlot->setAntialiasedElements(QCP::aeAll);
    customPlot->setNotAntialiasedElements(QCP::aeAll);
    // 图例名称显示
    customPlot->legend->setVisible(true);
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
    QSharedPointer<QCPAxisTickerFixed> axisTickerFixed(new QCPAxisTickerFixed);
    axisTickerFixed->setTickStep(256);//每间隔256单位一个标签
    axisTickerFixed->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);
    //axisTickerFixed->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssMeetTickCount);
    customPlot->xAxis->setTicker(axisTickerFixed);
    customPlot->xAxis->setRange(0, 4096);
    customPlot->yAxis->setRange(0, 10000);
    customPlot->yAxis->ticker()->setTickCount(5);
    customPlot->xAxis->ticker()->setTickCount(16);
    //customPlot->xAxis->ticker()->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssMeetTickCount);
    //customPlot->yAxis2->setPadding(10);//距离右边的距离

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
    customPlot->xAxis->setSubTicks(false);
    customPlot->xAxis2->setSubTicks(false);
    customPlot->yAxis->setSubTicks(false);
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

    // 文本元素随窗口变动而变动
//    titleTextTtem = new QCPItemText(customPlot);
//    titleTextTtem->setColor(clrLine);
//    titleTextTtem->position->setType(QCPItemPosition::ptAbsolute);
//    titleTextTtem->setPositionAlignment(Qt::AlignTop | Qt::AlignLeft);
//    titleTextTtem->setTextAlignment(Qt::AlignLeft);
//    titleTextTtem->setFont(QFont(font().family(), 12));
//    titleTextTtem->setPadding(QMargins(8, 0, 0, 0));
//    titleTextTtem->position->setCoords(10.0, 10.0);//窗口坐标值
//    titleTextTtem->setText(QString("%1").arg(title));
//    titleTextTtem->setVisible(false);

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
    coordsTextItem = new QCPItemText(customPlot);
    coordsTextItem->setColor(QColor(0,128,128,255));// 字体色
    coordsTextItem->setPen(QColor(130,130,130,255));// 边框
    coordsTextItem->setBrush(QBrush(QColor(255,255,225,255)));// 背景色
    coordsTextItem->setPositionAlignment(Qt::AlignBottom | Qt::AlignLeft);
    coordsTextItem->position->setType(QCPItemPosition::ptPlotCoords);
    coordsTextItem->setTextAlignment(Qt::AlignLeft);
    coordsTextItem->setFont(QFont(font().family(), 12));
    coordsTextItem->setPadding(QMargins(5, 5, 5, 5));
    coordsTextItem->position->setCoords(250.0, 1000.0);//X、Y轴坐标值
    coordsTextItem->setText("峰: 239.10 = 207.37 keV\n"
                            "半高宽: 40.15 FW[1/5]M:71.59\n"
                            "库: Sn-113[Tin]在255.04; 0 Bq\n"
                            "总计数面积: 623036\n"
                            "净面积: 57994 +/-3703\n"
                            "总计数/净计数 比率:731.18/68.06 cps");
    coordsTextItem->setVisible(false);

    textTipItem = new QCPItemText(customPlot);
    textTipItem->setPositionAlignment(Qt::AlignBottom|Qt::AlignLeft);
    textTipItem->position->setType(QCPItemPosition::ptAbsolute);
    textTipItem->setFont(QFont(font().family(), 15));
    textTipItem->setPen(QPen(Qt::black));
    textTipItem->setBrush(Qt::white);
    textTipItem->setVisible(false);

    lineFlagItem = new QCPItemLine(customPlot);
    lineFlagItem->setAntialiased(false);
    lineFlagItem->start->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    lineFlagItem->end->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    lineFlagItem->setPen(QPen(QColor(255,0,255,255), 3, Qt::SolidLine));
    lineFlagItem->setSelectedPen(QPen(QColor(255,0,255,255), 3, Qt::SolidLine));
    lineFlagItem->setVisible(false);

    dragRectItem = new QCPItemRect(customPlot);
    dragRectItem->setAntialiased(false);
    dragRectItem->topLeft->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    dragRectItem->bottomRight->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    dragRectItem->setPen(QPen(clrSelect, 2, Qt::DotLine));// 边框
    dragRectItem->setBrush(Qt::NoBrush);// 背景色
    dragRectItem->setSelectedPen(QPen(clrSelect, 2, Qt::DotLine));
    dragRectItem->setVisible(false);

    connect(customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*, int, QMouseEvent*)), this, SLOT(slotPlotClick(QCPAbstractPlottable*, int, QMouseEvent*)));
    //connect(customPlot, SIGNAL(selectionChangedByUser()), this, SLOT(slotSelectionChanged()));
    connect(customPlot, SIGNAL(beforeReplot()), this, SLOT(slotBeforeReplot()));
    connect(customPlot, SIGNAL(afterLayout()), this, SLOT(slotBeforeReplot()));

    updateShowModel(false);
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

void PlotWidget::initDetailWidget()
{
    auto initTableWidget = [=](QTableWidget* tableWidget){
        tableWidget->setColumnCount(2);
        tableWidget->setRowCount(8);
        tableWidget->horizontalHeader()->setVisible(false);
        tableWidget->verticalHeader()->setVisible(false);
        tableWidget->horizontalHeader()->setHighlightSections(false);

        //最后一列自适应宽度
        tableWidget->horizontalHeader()->setStretchLastSection(true);
        tableWidget->setColumnWidth(0, 80);
        tableWidget->setColumnWidth(1, 100);
        tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        /*隐藏网格线*/
        //tableWidget->setGridStyle(Qt::NoPen);

        //表格禁止编辑
        tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

        /*隐藏滚动条线*/
        tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        tableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        tableWidget->setFocusPolicy(Qt::NoFocus);
//        tableWidget->setStyleSheet("QTableView {background-color: #FFFF00;selection-background-color: #FFFF00; selection-color: #FFFF00;}"
//                                   "QTableView::horizontalScrollBar { width: 0px;}"
//                                   "QTableView::verticalScrollBar { height: 0px;}");
        tableWidget->setItem(0, 0, new QTableWidgetItem(QObject::tr("光标位置")));
        tableWidget->setItem(1, 0, new QTableWidgetItem(QObject::tr("Det1能量")));
        tableWidget->setItem(2, 0, new QTableWidgetItem(QObject::tr("Det2能量")));
        tableWidget->setItem(3, 0, new QTableWidgetItem(QObject::tr("计    数")));
        tableWidget->setItem(4, 0, new QTableWidgetItem(QObject::tr("道址量程")));
        tableWidget->setItem(5, 0, new QTableWidgetItem(QObject::tr("峰位道址")));
        tableWidget->setItem(6, 0, new QTableWidgetItem(QObject::tr("加亮区总计数")));
        tableWidget->setItem(7, 0, new QTableWidgetItem(QObject::tr("加亮区净计数")));
        //tableWidget->item(0, 2)->setTextAlignment(Qt::AlignCenter);
        //tableWidget->item(0, 3)->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);

        QVector<QColor> colors;
        colors << Qt::red << Qt::green << Qt::blue << Qt::black << Qt::yellow << Qt::lightGray;

        for (int i=0; i<8; i++){
            tableWidget->setRowHeight(i, 20);
            tableWidget->setItem(i, 1, new QTableWidgetItem(QObject::tr("")));
            tableWidget->item(i, 1)->setTextAlignment(Qt::AlignCenter);
            tableWidget->item(i, 0)->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        }
    };

    QWidget *detailWidget = new QWidget(this->customPlot);
    QVBoxLayout* layout = new QVBoxLayout(this->customPlot);
    detailWidget->setLayout(layout);
    detailWidget->layout()->setMargin(0);
    detailWidget->resize(200, 227);
    QTableWidget *detailTableWidget = new QTableWidget(detailWidget);
    detailWidget->layout()->addWidget(detailTableWidget);
    initTableWidget(detailTableWidget);

    detailWidget->setWindowOpacity(0.8);
    detailTableWidget->setWindowOpacity(0.8);
//    detailTableWidget->setAttribute(Qt::WA_TranslucentBackground);
//    detailTableWidget->setAttribute(Qt::WA_NoSystemBackground);
//    detailWidget->setAttribute(Qt::WA_TranslucentBackground);
//    detailWidget->setAttribute(Qt::WA_NoSystemBackground);
    detailWidget->setWindowFlags(Qt::FramelessWindowHint); // 移除窗口边框
    detailWidget->move(50, 15);
    detailWidget->show();

    QGroupBox *group = new QGroupBox(tr("高斯拟合"), this->customPlot);
    {
        QHBoxLayout *layout = new QHBoxLayout();
        group->setLayout(layout);
        QRadioButton *btn1 = new QRadioButton(tr("Det 1"), this->customPlot);
        btn1->setCheckable(true);
        btn1->setChecked(true);
        layout->addWidget(btn1);
        QRadioButton *btn2 = new QRadioButton(tr("Det 2"), this->customPlot);
        btn2->setCheckable(true);
        btn2->setChecked(false);
        layout->addWidget(btn2);

        QButtonGroup *grp = new QButtonGroup();
        grp->addButton(btn1, 0);
        grp->addButton(btn2, 1);
        connect(grp, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [=](int index){
            currentGraphIndex = index;
        });
    }
    detailWidget->layout()->addWidget(group);
    group->setFixedHeight(60);
    connect(customPlot, &QCustomPlot::afterLayout, this, [=](){
        //QPoint position = customPlot->axisRect()->topRight();
        //comboBox->move(position - QPoint(comboBox->width() - 1, 0));
        detailWidget->move(customPlot->axisRect()->topLeft() + QPoint(1, 0));
    });

    // 暂时未实现，先隐藏起来
    detailTableWidget->hide();
    detailWidget->setFixedHeight(60);
}

QCustomPlot *PlotWidget::customPlotInstance() const
{
    return this->customPlot;
}

#include <QMouseEvent>
bool PlotWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != this){
        if (event->type() == QEvent::MouseButtonPress){
            QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
            if (watched->inherits("QCustomPlot")){
                //QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(watched);
                if (e->button() == Qt::LeftButton) {
                    if (e->modifiers() & Qt::ControlModifier){
                        customPlot->setInteraction(QCP::iRangeDrag, false);
                    }

                    isPressed = true;
                    dragStart = e->pos();
                    customPlot->replot();
                } else if (e->button() == Qt::RightButton) {
                    // 右键恢复
                    isPressed = false;
                    isDragging = false;
                    unsigned int maxEngry = 0;

                    for(int index = 0; index<customPlot->graphCount(); ++index){
                        QCPGraph *graph = customPlot->graph(index);
                        QVector<double> keys, values;
                        QVector<QColor> colors;
                        int count = graph->data()->size();
                        for (int i=0; i<count; ++i){
                            keys << (double)graph->data()->at(i)->key;
                            values << (double)graph->data()->at(i)->value;
                            colors << clrLine[index];
                            maxEngry = (unsigned int)graph->data()->at(i)->value;
                        }

                        customPlot->graph(index)->setData(keys, values, colors);
                    }

                    if (customPlot->graphCount() >= 3)
                        customPlot->graph(2)->setVisible(false);
                    dragRectItem->setVisible(false);
                    coordsTextItem->setVisible(false);
                    lineFlagItem->setVisible(false);
                    customPlot->xAxis->setRange(0, qMax(4096, customPlot->graph(0)->data()->size()));
                    const QCPRange yRange = customPlot->yAxis->range();
                    maxEngry = (unsigned int)(((double)maxEngry) / 1000) * 1000 + 1000;
                    if (maxEngry != yRange.upper)
                        customPlot->yAxis->setRange(0, maxEngry);
                    customPlot->replot();

                    setCursor(Qt::ArrowCursor);
                }
            }
        } else if (event->type() == QEvent::MouseMove){
            QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
            if (watched->inherits("QCustomPlot")){
                //QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(watched);
                if (isPressed){
                    if (!isDragging) {
                        if (e->modifiers() & Qt::ControlModifier){
                            QCPGraph *graph = customPlot->graph(0);

                            dragRectItem->setProperty("top", dragStart.y());
                            dragRectItem->setProperty("left", dragStart.x());

                            double key, value;
                            graph->pixelsToCoords(dragStart.x(), dragStart.y(), key, value);
                            dragRectItem->topLeft->setCoords(key, value);

                            coordsTextItem->setVisible(false);
                            dragRectItem->setVisible(false);
                            customPlot->replot();

                            setCursor(Qt::CrossCursor);
                        } else {
                            setCursor(Qt::SizeAllCursor);
                        }

                        isDragging = true;
                    }else {
                        if (e->modifiers() & Qt::ControlModifier){
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

                /*
                // 当前鼠标位置（像素坐标）
                int x_pos = e->pos().x();
                int y_pos = e->pos().y();

                // 像素坐标转成实际的x,y轴的坐标
                double x_val = customPlot->xAxis->pixelToCoord(x_pos);
                double y_val = customPlot->yAxis->pixelToCoord(y_pos);

                double xMin = customPlot->xAxis->range().lower;
                double xMax = customPlot->xAxis->range().upper;
                double yMin = customPlot->yAxis->range().lower;
                double yMax = customPlot->yAxis->range().upper;
                if (x_val >= xMin && x_val <= xMax && y_val >= yMin && y_val <= yMax)
                {
                    for (int i = 0; i < customPlot->graph(0)->dataCount(); i++)
                    {
                        double key = abs(customPlot->graph(0)->dataMainKey(i) - x_val);
                        if ( key <= 60) {
                            if (lastIdx == i)
                                break;

                            lastIdx = i;
                            lineLeft->point1->setCoords(ui->graphicsView_history->graph(0)->dataMainKey(i), ui->graphicsView_history->yAxis->range().lower);
                            lineLeft->point2->setCoords(ui->graphicsView_history->graph(0)->dataMainKey(i), ui->graphicsView_history->yAxis->range().upper);
                        }
                    }
                }
                */
            }
        } else if (event->type() == QEvent::MouseButtonRelease){
            QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
            if (watched->inherits("QCustomPlot")){
                //QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(watched);
                if (e->button() == Qt::LeftButton) {
                    setCursor(Qt::ArrowCursor);

                    if (e->modifiers() & Qt::ControlModifier){
                        if (dragRectItem->visible()){
                            //颜色更新
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
                                if (graph->data()->at(i)->key>=key_from && graph->data()->at(i)->key<=key_to && graph->data()->at(i)->value>=value_to) {
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

                            // 高斯拟合
                            {
                                slotGauss(key_from, key_to);
                                double result[3];
                                fit_GaussCurve(fcount, sx, sy, result);

//                                QString info = QString("半高宽：%1\n峰: %2 = %3")
//                                        .arg(QString::number(result[0], 'f', 3))
//                                        .arg(QString::number(result[1], 'f', 0))
//                                        .arg(QString::number(result[2], 'f', 3));
//                                //显示拟合数据
////                              coordsTextItem->setText("峰: 239.10 = 207.37 keV\n"
////                                                        "半高宽: 40.15 FW[1/5]M:71.59\n"
////                                                        "库: Sn-113[Tin]在255.04; 0 Bq\n"
////                                                        "总计数面积: 623036\n"
////                                                        "净面积: 57994 +/-3703\n"
////                                                        "总计数/净计数 比率:731.18/68.06 cps");
//                                coordsTextItem->setText(info);
//                                double key, value;
//                                graph->pixelsToCoords(e->pos().x(), e->pos().y(), key, value);
//                                coordsTextItem->position->setCoords(key, value);
//                                coordsTextItem->setVisible(true);

//                                //绘制拟合曲线
//                                QCPGraph *graphCurve = customPlot->graph(2);
//                                QVector<double> curveValues;
//                                double a = result[2];
//                                double u = result[1];
//                                double FWHM = result[0];
//                                double ln2 = log(2);
//                                for (int i=0; i<curveKeys.size(); ++i){
//                                    //a*exp[-4ln2(x-u)^2/FWHM^2]，a=result[2],u=result[1],FWHM=result[0].
//                                    double x = curveKeys[i];
//                                    double v = a*exp(-4*ln2*pow(x-u,2)/pow(FWHM, 2));

//                                    curveValues.push_back(v);
//                                }
//                                graphCurve->setData(curveKeys, curveValues);
//                                graphCurve->setVisible(true);
//                                customPlot->replot();

                                //计算符合能窗
                                unsigned int minMean = (result[1] - result[0] / 2);
                                unsigned int maxMean = (result[1] + result[0] / 2);
                                emit sigUpdateMeanValues(currentGraphIndex, minMean, maxMean);
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
    QCPGraph *graph = qobject_cast<QCPGraph*>(plottable);//customPlot->graph(0);
    QSharedPointer<QCPGraphDataContainer> data = graph->data();
    const QCPGraphData *ghd = data->at(dataIndex);

    //显示轴信息
    QString text = QString::number(ghd->key,10,0) + "," + QString::number(ghd->value,10,0);
    textTipItem->setText(text);
    textTipItem->position->setCoords(event->pos().x() + 3, event->pos().y() - 3);
    textTipItem->setVisible(true);

    //显示标记点
    lineFlagItem->setProperty("key", ghd->key);
    lineFlagItem->setProperty("value", ghd->value);
    lineFlagItem->setVisible(true);

    customPlot->replot();

    setCursor(Qt::ArrowCursor);
    QTimer::singleShot(1000, this, [=](){
       textTipItem->setVisible(false);
       customPlot->replot();
    });
}

void PlotWidget::slotBeforeReplot()
{
    if (lineFlagItem->visible()){
        QCPGraph *graph = customPlot->graph(0);
        double key = lineFlagItem->property("key").toUInt();
        double value = lineFlagItem->property("value").toUInt();
        QPointF pixels = graph->coordsToPixels(key, value);
        graph->pixelsToCoords(pixels.x() + 1, pixels.y() - 2, key, value);
        lineFlagItem->start->setCoords(key, value);
        graph->pixelsToCoords(pixels.x() + 1, pixels.y() - 32, key, value);
        lineFlagItem->end->setCoords(key, value);
    }
}

void PlotWidget::slotSelectionChanged() {
    QList<QCPAbstractPlottable*> selectedPlottables = customPlot->selectedPlottables();
    foreach (QCPAbstractPlottable *plottable, selectedPlottables) {
        if (QCPCurve *curve = qobject_cast<QCPCurve*>(plottable)) {
            curve->setPen(QPen(Qt::red));
        } else if (QCPGraph *graph = qobject_cast<QCPGraph*>(plottable)) {
            graph->setPen(QPen(Qt::red));
        }
    }

    customPlot->replot(); // 重新绘制图形以显示更改
}

void PlotWidget::slotUpdateCountData(PariticalCountFrame frame)
{
    QCustomPlot *customPlot = customPlotInstance();
    int channelIndex = frame.channel;
    QVector<double> keys, values;
    QVector<QColor> colors;
    for (size_t i=0; i<frame.timeCountRate.size(); ++i){
        keys << i * frame.stepT;
        values << frame.timeCountRate[i].CountRates;
        colors << clrLine[channelIndex];
    }

    //customPlot->xAxis->setRange(0, frame.timeCountRate.size());
    //customPlot->yAxis->rescale(true);
    customPlot->graph(REF_GRAPH)->addData(keys, values, colors);
    customPlot->replot();
}

void PlotWidget::slotUpdateSpectrumData(PariticalSpectrumFrame frame)
{
    QCustomPlot *customPlot = customPlotInstance();
    int channelIndex = frame.channel;
    QVector<double> keys, values;
    QVector<QColor> colors;
    currentFrame[channelIndex].data.resize(frame.data.size());
    unsigned int maxEngry = 0;
    for (size_t i=0; i<frame.data.size(); ++i){
        keys << i;
        //values << frame.data[i];
        currentFrame[channelIndex].data[i] += frame.data[i];//将能量叠加
        values << currentFrame[channelIndex].data.at(i);
        colors << clrLine[channelIndex];

        maxEngry = qMax((unsigned int)maxEngry, (unsigned int)currentFrame[channelIndex].data.at(i));
    }

    //maxEngry = 10000;
    //动态调整轴范围
    {
//        const QCPRange xRange = customPlot->xAxis->range();
//        if ((xRange.upper - xRange.lower) != frame.data.size())
//            customPlot->xAxis->setRange(0, frame.data.size());

//        const QCPRange yRange = customPlot->yAxis->range();
//        maxEngry = (unsigned int)(((double)maxEngry) / 10000) * 10000 + 10000;
//        if (maxEngry != yRange.upper)
//            customPlot->yAxis->setRange(0, maxEngry);
        //customPlot->yAxis->rescale(false);
    }
    customPlot->graph(ENGRY_GRAPH)->setData(keys, values, colors);
    customPlot->replot();
}

void PlotWidget::slotResetPlot()
{
    for (int i=0; i<GRAPH_COUNT; ++i){
        currentFrame[i].data.clear();
    }
}

void PlotWidget::updateShowModel(bool refModel)
{
    customPlot->graph(ENGRY_GRAPH)->setVisible(!refModel);
    customPlot->graph(GAUSS_GRAPH)->setVisible(!refModel);
    customPlot->graph(REF_GRAPH)->setVisible(refModel);
}

void PlotWidget::slotGauss(int leftE, int rightE)
{
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
        QCPGraph *graphCurve = customPlot->graph(2);
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
