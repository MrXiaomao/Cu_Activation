#include "plotwidget.h"
#include "qcustomplot.h"
#include <QGridLayout>
#include <QtMath>
#include <cmath>

#define RANGE_SCARRE_UPPER 0.8
#define RANGE_SCARRE_LOWER 0.4
#define X_AXIS_LOWER    0
#define Y_AXIS_LOWER    -1

PlotWidget::PlotWidget(QWidget *parent) : QWidget(parent)
{
    this->setContentsMargins(0, 0, 0, 0);

    //计数模式下，坐标轴默认范围值
    COUNT_X_AXIS_LOWER[0] = 0; //计数率
    COUNT_X_AXIS_LOWER[1] = 0;//符合计数率
    COUNT_X_AXIS_UPPER[0] = 10*60;
    COUNT_X_AXIS_UPPER[1] = 10*60;//180秒

    COUNT_Y_AXIS_LOWER[0] = 0;
    COUNT_Y_AXIS_LOWER[1] = 0;
    COUNT_Y_AXIS_UPPER[0] = 1e4;
    COUNT_Y_AXIS_UPPER[1] = 1e3;

    //能谱模式下
    SPECTRUM_X_AXIS_LOWER = 0;
    SPECTRUM_X_AXIS_UPPER = MULTI_CHANNEL;
    SPECTRUM_Y_AXIS_LOWER = 0;
    SPECTRUM_Y_AXIS_UPPER = 1e4;

    this->setProperty("isMergeMode", false);
    this->setProperty("SelectDetIndex", 0);
}

void PlotWidget::resetAxisCoords()
{
    if (this->property("isMergeMode").toBool()){
        QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        if (this->property("isRefModel").toBool()){
            customPlotDet12->xAxis->setRange(COUNT_X_AXIS_LOWER[0], COUNT_X_AXIS_UPPER[0]); // 设置x轴的显示范围
            customPlotCoincidenceResult->xAxis->setRange(COUNT_X_AXIS_LOWER[1], COUNT_X_AXIS_UPPER[1]); // 设置x轴的显示范围

            customPlotDet12->yAxis->setRange(COUNT_Y_AXIS_LOWER[0], COUNT_Y_AXIS_UPPER[0]);
            customPlotCoincidenceResult->yAxis->setRange(COUNT_Y_AXIS_LOWER[1], COUNT_Y_AXIS_UPPER[1]);
        } else {
            customPlotDet12->xAxis->setRange(SPECTRUM_X_AXIS_LOWER, SPECTRUM_X_AXIS_UPPER); // 设置x轴的显示范围
            customPlotDet12->yAxis->setRange(SPECTRUM_Y_AXIS_LOWER, SPECTRUM_Y_AXIS_UPPER);
        }
    } else {
        QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
        QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        if (this->property("isRefModel").toBool()){
            customPlotDet1->xAxis->setRange(COUNT_X_AXIS_LOWER[0], COUNT_X_AXIS_UPPER[0]); // 设置x轴的显示范围
            customPlotDet2->xAxis->setRange(COUNT_X_AXIS_LOWER[0], COUNT_X_AXIS_UPPER[0]); // 设置x轴的显示范围
            customPlotCoincidenceResult->xAxis->setRange(COUNT_X_AXIS_LOWER[1], COUNT_X_AXIS_UPPER[1]); // 设置x轴的显示范围

            customPlotDet1->yAxis->setRange(COUNT_Y_AXIS_LOWER[0], COUNT_Y_AXIS_UPPER[0]);
            customPlotDet2->yAxis->setRange(COUNT_Y_AXIS_LOWER[0], COUNT_Y_AXIS_UPPER[0]);
            customPlotCoincidenceResult->yAxis->setRange(COUNT_Y_AXIS_LOWER[1], COUNT_Y_AXIS_UPPER[1]);
        } else {
            customPlotDet1->xAxis->setRange(SPECTRUM_X_AXIS_LOWER, SPECTRUM_X_AXIS_UPPER); // 设置x轴的显示范围
            customPlotDet2->xAxis->setRange(SPECTRUM_X_AXIS_LOWER, SPECTRUM_X_AXIS_UPPER); // 设置x轴的显示范围
            customPlotDet1->yAxis->setRange(SPECTRUM_Y_AXIS_LOWER, SPECTRUM_Y_AXIS_UPPER);
            customPlotDet2->yAxis->setRange(SPECTRUM_Y_AXIS_LOWER, SPECTRUM_Y_AXIS_UPPER);
        }
    }
}


void PlotWidget::rescalAxisCoords(QCustomPlot* customPlot)
{
    double maxEnergy = 0;
    double minKey = 0;

    // 先恢复数据颜色
    QCPGraph *graph = customPlot->graph(0);
    int count = graph->data()->size();
    if (count > 0)
        minKey = (double)graph->data()->at(0)->key;

    for (int i=0; i<count; ++i){
        maxEnergy = qMax(maxEnergy, (double)graph->data()->at(i)->value);
        minKey = qMin(minKey, (double)graph->data()->at(i)->key);
    }

    if (this->property("isRefModel").toBool()){
        customPlot->xAxis->setRange(minKey, minKey + COUNT_X_AXIS_UPPER[0]);//计数模式
        customPlot->yAxis->setRange(Y_AXIS_LOWER, COUNT_Y_AXIS_UPPER[0]/*maxEnergy / RANGE_SCARRE_LOWER*/);
    }
    else{
        customPlot->xAxis->setRange(minKey, minKey + SPECTRUM_X_AXIS_UPPER);//能谱模式
        customPlot->yAxis->setRange(Y_AXIS_LOWER, SPECTRUM_Y_AXIS_UPPER/*maxEnergy / RANGE_SCARRE_LOWER*/);
    }
}

QCPGraph* PlotWidget::getGraph(int index)
{
    //0-Det1 1-Det2 2=符合曲线
    QCPGraph* graph = nullptr;
    if (this->property("isMergeMode").toBool()){
        if (index == 0){
            QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
            graph = customPlotDet12->graph(0);
        } else if (index == 1){
            QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
            graph = customPlotDet12->graph(1);
        } else if (index == 2){
            QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");
            graph = customPlotCoincidenceResult->graph(0);
        } else if (index == 3){
            QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
            graph = customPlotDet12->graph(2);
        }
    } else {
        if (index == 0){
            QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
            graph = customPlotDet1->graph(0);
        } else if (index == 1){
            QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
            graph = customPlotDet2->graph(0);
        } else if (index == 2){
            QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");
            graph = customPlotCoincidenceResult->graph(0);
        } else if (index == 3){
            if (this->property("ActiveCustomPlotName") == "Det1"){
                QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
                graph = customPlotDet1->graph(1);
            } else {
                QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
                graph = customPlotDet2->graph(1);
            }
        }
    }

    return graph;
}

void PlotWidget::initCustomPlot(){
    QSplitter *spPlotWindow = new QSplitter(Qt::Vertical, this);
    spPlotWindow->setObjectName("spPlotWindow");

    if (this->property("isMergeMode").toBool()){
        QWidget *containWidget = new QWidget(this);
        containWidget->setObjectName("containWidgetDet12");
        containWidget->setLayout(new QVBoxLayout());
        containWidget->layout()->setContentsMargins(0, 0, 0, 0);
        QWidget* title = new QWidget();
        title->setLayout(new QHBoxLayout());
        title->layout()->setContentsMargins(0, 0, 0, 0);
        title->layout()->setSpacing(6);
        title->setFixedHeight(13);
        QLabel* label = new QLabel("Det1 / Det2");
        label->setContentsMargins(9, 0, 0, 0);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        title->layout()->addWidget(label);
        QRadioButton* det1 = new QRadioButton("Det1");
        det1->setCheckable(true);
        det1->setChecked(true);
        QString sColor = QString("color:%1").arg(clrLine.name());
        det1->setStyleSheet(sColor);
        QRadioButton* det2 = new QRadioButton("Det2");
        det2->setCheckable(true);
        det2->setStyleSheet(QString("color:%1").arg(clrLine2.name()));
        title->layout()->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
        title->layout()->addWidget(det1);
        title->layout()->addWidget(det2);
        QButtonGroup* grp = new QButtonGroup();
        grp->setExclusive(true);
        grp->addButton(det1, 0);
        grp->addButton(det2, 1);
        connect(grp, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [=](int index){
            if (0 == index)
                this->setProperty("ActiveCustomPlotName", "Det1");
            else
                this->setProperty("ActiveCustomPlotName", "Det2");
        });
        emit grp->buttonClicked(0);
        containWidget->layout()->addWidget(title);
        QCustomPlot *customPlot = allocCustomPlot("Det12", spPlotWindow);
        containWidget->layout()->addWidget(customPlot);
        spPlotWindow->addWidget(containWidget);
        customPlot->installEventFilter(this);

        dispatchAdditionalTipFunction(customPlot);
        dispatchAdditionalDragFunction(customPlot);
    } else {
        {
            QWidget *containWidget = new QWidget(this);
            containWidget->setObjectName("containWidgetDet1");
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

            dispatchAdditionalTipFunction(customPlot);
            dispatchAdditionalDragFunction(customPlot);
        }

        {
            QWidget *containWidget = new QWidget(this);
            containWidget->setObjectName("containWidgetDet2");
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

            dispatchAdditionalTipFunction(customPlot);
            dispatchAdditionalDragFunction(customPlot);
        }
    }

    {
        QWidget *containWidget = new QWidget(this);
        containWidget->setObjectName("containWidgetCoincidenceResult");
        containWidget->setLayout(new QVBoxLayout());
        containWidget->layout()->setContentsMargins(0, 0, 0, 0);
        QLabel* label = new QLabel(tr("符合结果"));
        label->setObjectName("labelCoincidenceResult");
        label->setContentsMargins(9, 0, 0, 0);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        //label->setFixedHeight(titleHeight);
        //label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        containWidget->layout()->addWidget(label);
        QCustomPlot *customPlot = allocCustomPlot("CoincidenceResult", spPlotWindow);
        containWidget->layout()->addWidget(customPlot);
        spPlotWindow->addWidget(containWidget);
        customPlot->installEventFilter(this);

        dispatchAdditionalTipFunction(customPlot);
    }

    spPlotWindow->setStretchFactor(0, 1);
    spPlotWindow->setStretchFactor(1, 1);
    spPlotWindow->setStretchFactor(2, 1);

    this->setLayout(new QVBoxLayout(this));
    this->layout()->setContentsMargins(0, 5, 0, 0);
    this->layout()->addWidget(spPlotWindow);

    switchShowModel(false);
    switchDataModel(false);
}

void PlotWidget::dispatchAdditionalTipFunction(QCustomPlot *customPlot)
{
    // 文本元素随坐标变动而变动
    QCPItemText *coordsTipItemText = new QCPItemText(customPlot);
    coordsTipItemText->setObjectName("coordsTipItemText");
    coordsTipItemText->setPositionAlignment(Qt::AlignBottom|Qt::AlignLeft);
    coordsTipItemText->position->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    coordsTipItemText->setFont(QFont(font().family(), 15));
    coordsTipItemText->setPen(QPen(Qt::black));
    coordsTipItemText->setBrush(Qt::white);
    coordsTipItemText->setVisible(false);

    QCPItemLine *flagItemLine = new QCPItemLine(customPlot);
    flagItemLine->setObjectName("flagItemLine");
    flagItemLine->setAntialiased(false);
    flagItemLine->start->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    flagItemLine->end->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    flagItemLine->setPen(QPen(QColor(255,0,255,255), 2, Qt::SolidLine));
    flagItemLine->setSelectedPen(QPen(QColor(255,0,255,255), 2, Qt::SolidLine));
    flagItemLine->setVisible(false);

    QCPItemRect *dragRectItem = new QCPItemRect(customPlot);
    dragRectItem->setObjectName("dragRectItem");
    dragRectItem->setAntialiased(false);
    dragRectItem->topLeft->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    dragRectItem->bottomRight->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    dragRectItem->setPen(QPen(clrSelect, 1, Qt::DotLine));// 边框
    dragRectItem->setBrush(Qt::NoBrush);// 背景色
    dragRectItem->setSelectedPen(QPen(clrSelect, 1, Qt::DotLine));
    dragRectItem->setVisible(false);
}

void PlotWidget::dispatchAdditionalDragFunction(QCustomPlot *customPlot)
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

    QCPItemRect *dragRectItem = new QCPItemRect(customPlot);
    dragRectItem->setObjectName("dragRectItem");
    dragRectItem->setAntialiased(false);
    dragRectItem->topLeft->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    dragRectItem->bottomRight->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    dragRectItem->setPen(QPen(clrSelect, 1, Qt::DotLine));// 边框
    dragRectItem->setBrush(Qt::NoBrush);// 背景色
    dragRectItem->setSelectedPen(QPen(clrSelect, 1, Qt::DotLine));
    dragRectItem->setVisible(false);

    auto createStraightLineItem = [=](QCPItemStraightLine** _line){
        QCPItemStraightLine *line = new QCPItemStraightLine(customPlot);
        line->setLayer("overlay");
        line->setPen(QPen(Qt::gray, 1, Qt::DotLine));
        line->setClipToAxisRect(true);
        line->point1->setCoords(0, 0);
        line->point2->setCoords(0, 0);
        line->setVisible(true);
        *_line = line;
    };
    QCPItemStraightLine *itemStraightLineLeft, *itemStraightLineRight;
    createStraightLineItem(&itemStraightLineLeft);
    itemStraightLineLeft->setObjectName("itemStraightLineLeft");
    createStraightLineItem(&itemStraightLineRight);
    itemStraightLineRight->setObjectName("itemStraightLineRight");
}


QCustomPlot *PlotWidget::allocCustomPlot(QString objName, QWidget *parent)
{
    QCustomPlot* customPlot = new QCustomPlot(parent);
    customPlot->setObjectName(objName);
    //customPlot->setOpenGl(true); //不能启用硬件加速，否则多个控件数据刷新起冲突
//    customPlot->installEventFilter(this);
//    customPlot->plotLayout()->insertRow(0);
//    customPlot->plotLayout()->addElement(0, 0, new QCPTextElement(customPlot, title, QFont("微软雅黑", 10, QFont::Bold)));
    // 设置选择容忍度，即鼠标点击点到数据点的距离
    //customPlot->setSelectionTolerance(5);
    // 设置全局抗锯齿
    customPlot->setAntialiasedElements(QCP::aeAll);
    //customPlot->setNotAntialiasedElements(QCP::aeAll);
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
//    customPlot->xAxis->setRange(0, 180);
//    customPlot->yAxis->setRange(Y_AXIS_LOWER, 10000);
    customPlot->yAxis->ticker()->setTickCount(5);
    customPlot->xAxis->ticker()->setTickCount(16);

    customPlot->yAxis2->ticker()->setTickCount(5);
    customPlot->xAxis2->ticker()->setTickCount(16);
    // 设置刻度可见
    customPlot->xAxis->setTicks(axisVisible);
    customPlot->xAxis2->setTicks(axisVisible);
    customPlot->yAxis->setTicks(axisVisible);
    customPlot->yAxis2->setTicks(axisVisible);
    // 设置刻度高度
    customPlot->xAxis->setTickLength(17);
    customPlot->yAxis->setTickLength(17);
    customPlot->xAxis->setSubTickLength(4);
    customPlot->yAxis->setSubTickLength(4);

    customPlot->xAxis2->setTickLength(17);
    customPlot->yAxis2->setTickLength(17);
    customPlot->xAxis2->setSubTickLength(4);
    customPlot->yAxis2->setSubTickLength(4);
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
    //customPlot->xAxis->setLabel(QObject::tr("时间"));
    //customPlot->yAxis->setLabel(QObject::tr("能量/Kev"));
    // 设置网格线颜色
    customPlot->xAxis->grid()->setPen(QPen(QColor(114, 114, 114, 255), 1, Qt::PenStyle::DashLine));
    customPlot->yAxis->grid()->setPen(QPen(QColor(114, 114, 114, 255), 1, Qt::PenStyle::DashLine));
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

    this->setProperty("autoRefreshModel", true);
    customPlot->xAxis->rescale(true);
    customPlot->yAxis->rescale(true);



    // 添加散点图
    int count = 1;
    if (this->property("isMergeMode").toBool()){
        count = 2;
    }

    for (int i=0; i<count; ++i){
        QCPGraph * mainGraph = customPlot->addGraph(customPlot->xAxis, customPlot->yAxis);
        mainGraph->setName("mainGraph");
        mainGraph->setAntialiased(false);
        mainGraph->setPen(QPen(QColor(i == 0x00 ? clrLine : clrLine2)));
        mainGraph->selectionDecorator()->setPen(QPen(i == 0x00 ? clrLine : clrLine2));
        mainGraph->setLineStyle(QCPGraph::lsNone);// 隐藏线性图
        mainGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 5));//显示散点图
    }

    if (objName != "CoincidenceResult"){
        // 添加高斯曲线图
        QCPGraph *curveGraph = customPlot->addGraph(customPlot->xAxis, customPlot->yAxis);
        curveGraph->setName("curveGraph");
        curveGraph->setAntialiased(true);
        curveGraph->setPen(QPen(clrGauseCurve));
        curveGraph->selectionDecorator()->setPen(QPen(clrGauseCurve));
        curveGraph->setLineStyle(QCPGraph::lsLine);
        curveGraph->setVisible(false);
    }

    //connect(customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*, int, QMouseEvent*)), this, SLOT(slotPlotClick(QCPAbstractPlottable*, int, QMouseEvent*)));
    connect(customPlot, SIGNAL(beforeReplot()), this, SLOT(slotBeforeReplot()));
    connect(customPlot, SIGNAL(afterLayout()), this, SLOT(slotBeforeReplot()));

    connect(customPlot->xAxis, SIGNAL(rangeChanged(const QCPRange &)), customPlot->xAxis2, SLOT(setRange(const QCPRange &)));
    connect(customPlot->yAxis, SIGNAL(rangeChanged(const QCPRange &)), customPlot->yAxis2, SLOT(setRange(const QCPRange &)));
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

        QCPItemStraightLine* itemStraightLineLeft = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineLeft");
        if (itemStraightLineLeft){
            QPointF coords = itemStraightLineLeft->point1->coords();
            itemStraightLineLeft->point1->setCoords(coords.x(), range.lower);
            itemStraightLineLeft->point2->setCoords(coords.x(), range.upper);
        }
        QCPItemStraightLine* itemStraightLineRight = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineRight");
        if (itemStraightLineRight){
            QPointF coords = itemStraightLineRight->point1->coords();
            itemStraightLineRight->point1->setCoords(coords.x(), range.lower);
            itemStraightLineRight->point2->setCoords(coords.x(), range.upper);
        }
    });

    connect(customPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(slotRestorePlot()));
    connect(customPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(slotRestorePlot()));
    connect(customPlot, SIGNAL(mouseWheel(QMouseEvent*)), this, SLOT(slotRestorePlot()));

    if (objName == "CoincidenceResult"){
        QTimer *timerUpdate = new QTimer(this);
        static int timerRef = 0;
        connect(timerUpdate, &QTimer::timeout, this, [=](){
            // 更新数据
            customPlot->graph(0)->addData(timerRef++, 0, Qt::blue);

//            if (customPlot->xAxis->range().upper > 180){
//                customPlot->xAxis->setRange(customPlot->xAxis->range().upper - 180, customPlot->xAxis->range().upper);
//            }

            if (customPlot->graph(0)->data()->size() > 180){
                customPlot->xAxis->setRange(timerRef - 180, timerRef);
            }

            // 重新绘制
            //customPlot->xAxis->rescale(true);
            customPlot->replot();
        });
        timerUpdate->start(1000);
    }

    return customPlot;
}

void PlotWidget::slotRestorePlot()
{
    QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(sender());
    if (this->property("autoRefreshModel").toBool())
        customPlot->xAxis->rescale(false);

    QTimer* timerAutoRestore = this->findChild<QTimer*>("timerAutoRestore");
    if (nullptr == timerAutoRestore){
        timerAutoRestore = new QTimer(this);
        connect(timerAutoRestore, &QTimer::timeout, this, [=](){
            //拖拽开始
            timerAutoRestore->stop();
            this->setProperty("autoRefreshModel", true);
            customPlot->xAxis->rescale(true);
        });
    }

    timerAutoRestore->start(30000);
}

void PlotWidget::resetPlotDatas(QCustomPlot* customPlot)
{
    //右键重设数据初始状态
    double maxEnergy = 0;
    double minKey = 0;

    QMutexLocker locker(&mutexRefreshPlot);
    if (this->property("isMergeMode").toBool()){

    } else {
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

        if (customPlot->objectName() != "CoincidenceResult"){
            customPlot->graph(1)->data().clear();
            QCPItemRect* dragRectItem = customPlot->findChild<QCPItemRect*>("dragRectItem");//鼠标拖拽框
            if (dragRectItem)
                dragRectItem->setVisible(false);
            QCPItemText* gaussResultItemText = customPlot->findChild<QCPItemText*>("gaussResultItemText");//拟合信息
            if (gaussResultItemText)
                gaussResultItemText->setVisible(false);
            QCPItemLine* flagItemLine = customPlot->findChild<QCPItemLine*>("flagItemLine");//点选标记竖线
            if (flagItemLine)
                flagItemLine->setVisible(false);
        }
    }
    //rescalAxisCoords(customPlot);
    resetAxisCoords();
    customPlot->replot();
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
                    //符合结构不需要拖拽
                    if (customPlot->objectName() == "CoincidenceResult")
                        return QWidget::eventFilter(watched, event);

                    //计数模式不需要拖拽
                    if (this->property("isRefModel").toBool())
                        return QWidget::eventFilter(watched, event);

                    enableDrag = false;
                    if (e->modifiers() & Qt::ControlModifier){//Ctrl键按下，禁止拖拽功能
                        customPlot->setInteraction(QCP::iRangeDrag, false);
                        setCursor(Qt::CrossCursor);

                    } else {
                        QTimer* timerRef = this->findChild<QTimer*>("timerRef");
                        if (nullptr == timerRef){
                            timerRef = new QTimer(this);
                            timerRef->setObjectName("timerRef");
                            connect(timerRef, &QTimer::timeout, this, [=](){
                                //拖拽开始
                                timerRef->stop();
                                setCursor(Qt::CrossCursor);
                                customPlot->setInteraction(QCP::iRangeDrag, false);
                                enableDrag = true;
                            });
                        }

                        timerRef->start(500);
                    }

                    isPressed = true;
                    dragStart = e->pos();
                } else if (e->button() == Qt::RightButton) {// 右键恢复
                    enableDrag = false;
                    isPressed = false;
                    isDragging = false;

                    customPlot->xAxis->rescale(true);
                    customPlot->yAxis->rescale(true);
                    //resetPlotDatas(customPlot);//右键重设数据初始状态
                    setCursor(Qt::ArrowCursor);
                }
            }
        } else if (event->type() == QEvent::MouseMove){
            QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
            if (watched->inherits("QCustomPlot")){
                QTimer* timerRef = this->findChild<QTimer*>("timerRef");
                if (timerRef)
                    timerRef->stop();

                QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(watched);
                if (isPressed){
                    //符合结果禁止拖拽
                    if (customPlot->objectName() == "CoincidenceResult")
                        return QWidget::eventFilter(watched, event);

                    if (!isDragging) {
                        if (enableDrag || (e->modifiers() & Qt::ControlModifier)){//鼠标框选开始
                            isDragging = true;

                            //拖拽开始，设置初始值
                            QCPItemStraightLine* itemStraightLineLeft = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineLeft");
                            if (itemStraightLineLeft){
                                itemStraightLineLeft->setVisible(false);
                            }
                            QCPItemStraightLine* itemStraightLineRight = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineRight");
                            if (itemStraightLineRight){
                                itemStraightLineRight->setVisible(false);
                            }

                            QMutexLocker locker(&mutexRefreshPlot);
                            QCPGraph *graph = getGraph(this->property("SelectDetIndex").toUInt());

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
                    }else {
                        if (isDragging) {
                            //拖拽中
                            QMutexLocker locker(&mutexRefreshPlot);

                            QCPItemRect* dragRectItem = customPlot->findChild<QCPItemRect*>("dragRectItem");
                            if (dragRectItem){
                                dragRectItem->setProperty("bottom", e->pos().y());
                                dragRectItem->setProperty("right", e->pos().x());

                                double key, value;
                                QCPGraph *graph = getGraph(this->property("SelectDetIndex").toUInt());
                                graph->pixelsToCoords(e->pos().x(), e->pos().y(), key, value);
                                dragRectItem->bottomRight->setCoords(key, value);
                                dragRectItem->setVisible(true);

                                customPlot->replot();
                            }
                        }
                    }
                } else {
                    // 显示坐标值
                    QMutexLocker locker(&mutexRefreshPlot);
                    QCPItemText* coordsTipItemText = customPlot->findChild<QCPItemText*>("coordsTipItemText");
                    if (coordsTipItemText){
                        //像素坐标转成实际的x,y轴的坐标
                        QCPGraph *graph = getGraph(this->property("SelectDetIndex").toUInt());
                        double x_val = customPlot->xAxis->pixelToCoord(e->pos().x());
                        //double y_val = customPlot->yAxis->pixelToCoord(e->pos().y());
                        //auto iter = graph->data()->findBegin(x_val);
                        //x_val = iter->mainKey();

                        QSharedPointer<QCPGraphDataContainer> data = graph->data();
                        const QCPGraphData *ghd = data->at(x_val);
                        if (ghd && ghd->key>0 && ghd->value>0){
                            //显示轴信息
                            QString text = QString::number(ghd->key,10,0) + "," + QString::number(ghd->value,10,0);
                            coordsTipItemText->setText(text);

                            //计算Y轴
                            coordsTipItemText->position->setCoords(ghd->key, ghd->value);
                            //coordsTipItemText->position->setCoords(e->pos().x() + 3, e->pos().y() - 3);
                            coordsTipItemText->setVisible(true);

                            //显示标记点
                            QCPItemLine* flagItemLine = customPlot->findChild<QCPItemLine*>("flagItemLine");
                            if (flagItemLine){
                                flagItemLine->setProperty("key", ghd->key);
                                flagItemLine->setProperty("value", ghd->value);
                                flagItemLine->setVisible(true);
                            }

                            customPlot->replot();

                            //3秒后自动隐藏
                            QTimer::singleShot(3000, this, [=](){
                                flagItemLine->setVisible(false);
                                coordsTipItemText->setVisible(false);
                                customPlot->replot();
                             });
                        } else {

                        }
                    }

                    setCursor(Qt::ArrowCursor);
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease){
            QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
            if (watched->inherits("QCustomPlot")){
                QTimer* timerRef = this->findChild<QTimer*>("timerRef");
                if (timerRef)
                    timerRef->stop();

                QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(watched);
                if (customPlot->objectName() == "CoincidenceResult")
                    return QWidget::eventFilter(watched, event);

                if (e->button() == Qt::LeftButton) { //松开鼠标左键
                    setCursor(Qt::ArrowCursor);

                    if (enableDrag || (e->modifiers() & Qt::ControlModifier)){//Ctrl按键
                        QCPItemRect* dragRectItem = customPlot->findChild<QCPItemRect*>("dragRectItem");
                        if (dragRectItem && dragRectItem->visible()){
                            QMutexLocker locker(&mutexRefreshPlot);

                            QCPGraph *graph = getGraph(this->property("SelectDetIndex").toUInt());
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

                                graph->setData(keys, values, colors);

                                //显示拖拽边界线
                                QCPItemStraightLine* itemStraightLineLeft = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineLeft");
                                if (itemStraightLineLeft){
                                    itemStraightLineLeft->point1->setCoords(key_from, customPlot->yAxis->range().lower);
                                    itemStraightLineLeft->point2->setCoords(key_from, customPlot->yAxis->range().upper);
                                    itemStraightLineLeft->setVisible(true);
                                }
                                QCPItemStraightLine* itemStraightLineRight = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineRight");
                                if (itemStraightLineRight){
                                    itemStraightLineRight->point1->setCoords(key_to, customPlot->yAxis->range().lower);
                                    itemStraightLineRight->point2->setCoords(key_to, customPlot->yAxis->range().upper);
                                    itemStraightLineRight->setVisible(true);
                                }

                                dragRectItem->setVisible(false);
                                customPlot->replot();
                            }

                            // 高斯拟合
                            if (fcount > 0){
                                //显示拟合曲线
                                double result[3];
                                if (fit_GaussCurve(fcount, sx, sy, result)){
                                    if (!std::isnan(result[0]) && !std::isnan(result[1]) && !std::isnan(result[2])){
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
                        }
                    }

                    isPressed = false;
                    isDragging = false;
                    enableDrag = false;

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
        QCPGraph *graph = qobject_cast<QCPGraph*>(plottable);
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

        QTimer::singleShot(3000, this, [=](){
            flagItemLine->setVisible(false);
            coordsTipItemText->setVisible(false);
            customPlot->replot();
        });
    }

    setCursor(Qt::ArrowCursor);
}

void PlotWidget::slotBeforeReplot()
{
    QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(sender());
    QCPItemLine* flagItemLine = customPlot->findChild<QCPItemLine*>("flagItemLine");
    if (flagItemLine && flagItemLine->visible()){
        QCPGraph *graph = getGraph(this->property("SelectDetIndex").toUInt());
        double key = flagItemLine->property("key").toUInt();
        double value = flagItemLine->property("value").toUInt();
        QPointF pixels = graph->coordsToPixels(key, value);
        graph->pixelsToCoords(pixels.x() + 1, pixels.y() - 2, key, value);
        flagItemLine->start->setCoords(key, value);
        graph->pixelsToCoords(pixels.x() + 1, pixels.y() - 32, key, value);
        flagItemLine->end->setCoords(key, value);
    }
}

#include <QtMath>
#include <math.h>
void PlotWidget::slotUpdatePlotDatas(vector<SingleSpectrum> r1, vector<CurrentPoint> r2, vector<CoincidenceResult> r3)
{
    QMutexLocker locker(&mutexRefreshPlot);

    if (this->property("isMergeMode").toBool()){
        QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
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

                customPlotDet12->graph(0)->setData(keys, values, colors);
                if (maxPoint > customPlotDet12->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotDet12->yAxis->setRange(customPlotDet12->yAxis->range().lower, maxPoint / RANGE_SCARRE_LOWER);
                }
            }

            {//Det2
                double maxPoint = 0.;
                QVector<double> keys, values;
                QVector<QColor> colors;
                for (size_t i=0; i<r2.size(); ++i){
                    keys << i+1;
                    values << r2[i].dataPoint2;
                    colors << clrLine2;
                    maxPoint = qMax(maxPoint, values[i]);
                }

                if (customPlotDet12->graph(1)->visible()){
                    customPlotDet12->graph(1)->setData(keys, values, colors);
                    if (maxPoint > customPlotDet12->yAxis->range().upper * RANGE_SCARRE_UPPER){
                        customPlotDet12->yAxis->setRange(customPlotDet12->yAxis->range().lower, maxPoint / RANGE_SCARRE_LOWER);
                    }
                }
                customPlotDet12->replot();
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

                customPlotDet12->graph(0)->setData(keys, values, colors);
                if (maxEnergy > customPlotDet12->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotDet12->yAxis->setRange(customPlotDet12->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
                }
            }

            {//Det2
                double maxEnergy = 0;
                QVector<double> keys, values;
                QVector<QColor> colors;

                for (size_t j=0; j<r1.size(); ++j){
                    for (int i=0; i<MULTI_CHANNEL; ++i){
                        energyValues[1][i] += r1[j].spectrum[1][i];
                    }
                }

                for (int i=0; i<MULTI_CHANNEL; ++i){
                    keys << i+1;
                    colors << clrLine;
                    values << energyValues[1][i];
                    maxEnergy = qMax(maxEnergy, values[i]);
                }

                customPlotDet12->graph(1)->setData(keys, values, colors);
                if (maxEnergy > customPlotDet12->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotDet12->yAxis->setRange(customPlotDet12->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
                }

                customPlotDet12->replot();
            }
        }
    } else {
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

                //if (this->property("autoRefreshModel").toBool())
                //    customPlotDet1->xAxis->rescale(true);
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
                        energyValues[1][i] += r1[j].spectrum[1][i];
                    }
                }

                for (int i=0; i<MULTI_CHANNEL; ++i){
                    keys << i+1;
                    colors << clrLine;
                    values << energyValues[1][i];
                    maxEnergy = qMax(maxEnergy, values[i]);
                }

                customPlotDet2->graph(0)->setData(keys, values, colors);
                if (maxEnergy > customPlotDet2->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotDet2->yAxis->setRange(customPlotDet2->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
                }

                customPlotDet2->replot();
            }
        }
    }
}

void PlotWidget::slotResetPlot()
{
    QMutexLocker locker(&mutexRefreshPlot);
    getGraph(0)->data()->clear();
    getGraph(1)->data()->clear();
    getGraph(2)->data()->clear();
    getGraph(3)->data()->clear();

    QVector<double>(MULTI_CHANNEL, 0).swap(energyValues[0]);
    QVector<double>(MULTI_CHANNEL, 0).swap(energyValues[1]);

    if (this->property("isMergeMode").toBool()){
        QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        customPlotDet12->replot();
        customPlotCoincidenceResult->replot();
    } else {
        QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
        QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        customPlotDet1->replot();
        customPlotDet2->replot();
        customPlotCoincidenceResult->replot();
    }
}

void PlotWidget::switchShowModel(bool refModel)
{
    QMutexLocker locker(&mutexRefreshPlot);
    this->setProperty("isRefModel", refModel);
    QWidget *containWidgetCoincidenceResult = this->findChild<QWidget*>("containWidgetCoincidenceResult");

    getGraph(0)->data()->clear();
    getGraph(1)->data()->clear();
    getGraph(2)->data()->clear();
    getGraph(3)->data()->clear();

    if (this->property("isMergeMode").toBool()){
        QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        if (this->property("isRefModel").toBool()){
            customPlotDet12->xAxis->setLabel(tr("时间/s"));
            customPlotDet12->yAxis->setLabel(tr("计数率/cps"));

            customPlotCoincidenceResult->xAxis->setLabel(tr("时间/s"));
            customPlotCoincidenceResult->yAxis->setLabel(tr("计数率/cps"));

            //高斯曲线隐藏
            getGraph(3)->setVisible(false);

            //符合结果隐藏
            containWidgetCoincidenceResult->setVisible(true);
        } else {
            customPlotDet12->xAxis->setLabel(tr("道址"));
            customPlotDet12->yAxis->setLabel(tr("计数"));

            //高斯曲线可见
            getGraph(3)->setVisible(true);

            //符合结果隐藏
            containWidgetCoincidenceResult->setVisible(false);
        }

        resetAxisCoords();

        customPlotDet12->replot();
        customPlotCoincidenceResult->replot();
    } else {
        QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
        QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        if (this->property("isRefModel").toBool()){
            customPlotDet1->xAxis->setLabel(tr("时间/s"));
            customPlotDet1->yAxis->setLabel(tr("计数率/cps"));
            customPlotDet2->xAxis->setLabel(tr("时间/s"));
            customPlotDet2->yAxis->setLabel(tr("计数率/cps"));

            customPlotCoincidenceResult->xAxis->setLabel(tr("时间/s"));
            customPlotCoincidenceResult->yAxis->setLabel(tr("计数率/cps"));

            customPlotDet1->graph(1)->setVisible(false);
            customPlotDet2->graph(1)->setVisible(false);

            containWidgetCoincidenceResult->setVisible(true);
        } else {
            customPlotDet1->xAxis->setLabel(tr("道址"));
            customPlotDet1->yAxis->setLabel(tr("计数"));
            customPlotDet2->xAxis->setLabel(tr("道址"));
            customPlotDet2->yAxis->setLabel(tr("计数"));

            customPlotDet1->graph(1)->setVisible(true);
            customPlotDet2->graph(1)->setVisible(true);

            containWidgetCoincidenceResult->setVisible(false);
        }

        resetAxisCoords();

        customPlotDet1->replot();
        customPlotDet2->replot();
        customPlotCoincidenceResult->replot();
    }
}

void PlotWidget::switchDataModel(bool isLogarithmic)
{
    QMutexLocker locker(&mutexRefreshPlot);
    this->setProperty("isLogarithmic", isLogarithmic);

    if (this->property("isMergeMode").toBool()){
        QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        if (isLogarithmic){
            QCustomPlot* customPlots[2] = {customPlotDet12, customPlotCoincidenceResult};
            for (auto customPlot : customPlots){
                customPlot->yAxis->setSubTicks(true);
                customPlot->yAxis2->setSubTicks(true);

                customPlot->yAxis->setScaleType(QCPAxis::ScaleType::stLogarithmic);
                customPlot->yAxis->setNumberFormat("eb");//使用科学计数法表示刻度
                customPlot->yAxis->setNumberPrecision(0);//小数点后面小数位数

                QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
                customPlot->yAxis->setTicker(logTicker);
                customPlot->replot();
            }
        } else {
            customPlotDet12->yAxis->setSubTicks(false);
            customPlotDet12->yAxis2->setSubTicks(false);
            customPlotCoincidenceResult->yAxis->setSubTicks(false);
            customPlotCoincidenceResult->yAxis2->setSubTicks(false);

            QCustomPlot* customPlots[2] = {customPlotDet12, customPlotCoincidenceResult};
            for (auto customPlot : customPlots){
                customPlot->yAxis->setSubTicks(false);
                customPlot->yAxis2->setSubTicks(false);

                QSharedPointer<QCPAxisTicker> ticker(new QCPAxisTicker);
                customPlot->yAxis->setTicker(ticker);

                customPlot->yAxis->setScaleType(QCPAxis::ScaleType::stLinear);
                customPlot->yAxis->setRange(0, 10000);
                customPlot->yAxis->setNumberFormat("f");
                customPlot->yAxis->setNumberPrecision(0);

                customPlot->replot();
            }
        }
    } else {
        QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
        QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        if (isLogarithmic){
            QCustomPlot* customPlots[3] = {customPlotDet1, customPlotDet2, customPlotCoincidenceResult};
            for (auto customPlot : customPlots){
                customPlot->yAxis->setSubTicks(true);
                customPlot->yAxis2->setSubTicks(true);

                customPlot->yAxis->setScaleType(QCPAxis::ScaleType::stLogarithmic);
                customPlot->yAxis->setNumberFormat("eb");//使用科学计数法表示刻度
                customPlot->yAxis->setNumberPrecision(0);//小数点后面小数位数

                QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
                customPlot->yAxis->setTicker(logTicker);
                customPlot->replot();
            }
        } else {
            QCustomPlot* customPlots[3] = {customPlotDet1, customPlotDet2, customPlotCoincidenceResult};
            for (auto customPlot : customPlots){
                customPlot->yAxis->setSubTicks(false);
                customPlot->yAxis2->setSubTicks(false);

                QSharedPointer<QCPAxisTicker> ticker(new QCPAxisTicker);
                customPlot->yAxis->setTicker(ticker);

                customPlot->yAxis->setScaleType(QCPAxis::ScaleType::stLinear);
                customPlot->yAxis->setRange(0, 10000);
                customPlot->yAxis->setNumberFormat("f");
                customPlot->yAxis->setNumberPrecision(0);

                customPlot->replot();
            }
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

        QCPGraph *graph = getGraph(this->property("SelectDetIndex").toUInt());//customPlot->graph(0);
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
            QCPGraph *curveGraph = customPlot->graph(1);
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

            curveGraph->setData(curveKeys, curveValues);
            curveGraph->setVisible(true);
            customPlot->replot();
        }
    }
}
