#include "plotwidget.h"
#include "qcustomplot.h"
#include <QGridLayout>
#include <QtMath>
#include <cmath>
#include "linearfit.h"

#define RANGE_SCARRE_UPPER 0.8
#define RANGE_SCARRE_LOWER 0.4
#define X_AXIS_LOWER    0
#define Y_AXIS_LOWER    0

PlotWidget::PlotWidget(QWidget *parent) : QWidget(parent)
{
    this->setContentsMargins(0, 0, 0, 0);

    //计数模式下，坐标轴默认范围值
    COUNT_X_AXIS_LOWER[0] = 0; //计数率
    COUNT_X_AXIS_LOWER[1] = 0;//符合计数率
    COUNT_X_AXIS_UPPER[0] = 60;
    COUNT_X_AXIS_UPPER[1] = 60;//60秒

    COUNT_Y_AXIS_LOWER[0] = 0;
    COUNT_Y_AXIS_LOWER[1] = 0;
    COUNT_Y_AXIS_UPPER[0] = 1e2;
    COUNT_Y_AXIS_UPPER[1] = 1e2;

    //能谱模式下
    SPECTRUM_X_AXIS_LOWER = 0;
    SPECTRUM_X_AXIS_UPPER = MAX_SPECTUM;
    SPECTRUM_Y_AXIS_LOWER = 0;
    SPECTRUM_Y_AXIS_UPPER = 1e2;

    /*合并模式：2个探测器在同一个图形上*/
    this->setProperty("isMergeMode", false);
    /*当前激活的探测器，默认值第一个*/
    this->setProperty("SelectDetIndex", 0);
    /*默认显示高斯信息*/
    this->setProperty("showGaussInfo", true);
    this->setProperty("autoRefreshModel", true);
    this->setProperty("refresh-time-range", 600);//默认时长10分钟：10*60
}

void PlotWidget::resetAxisCoords()
{
    if (this->property("isMergeMode").toBool()){
        QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        if (this->property("isCountMode").toBool()){
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

        if (this->property("isCountMode").toBool()){
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

    if (this->property("isCountMode").toBool()){
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

    QString styleSheet = "QLabel{"
            "background-color:qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #cbdcf1, stop: 0.5 #cddff4, stop: 1.0 #cddff4);"
            "border-top:2px solid;"
            "border-top-clor:#8da5c4;"
            "border-bottom:2px solid;"
            "border-bottom-color:#ccdbec;"
            "}";

    styleSheet = "";
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
        label->setStyleSheet(styleSheet);
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
        for (int i=1; i<=2; ++i){
            // QWidget *containWidget = new QWidget(this);
            // containWidget->setObjectName(QString("containWidgetDet%1").arg(i));
            // containWidget->setLayout(new QVBoxLayout());
            // containWidget->layout()->setContentsMargins(0, 0, 0, 0);
            // QHBoxLayout *hLayout = new QHBoxLayout(containWidget);
            // hLayout->setContentsMargins(0, 0, 0, 0);
            // hLayout->setSpacing(9);

            // QLabel* label = new QLabel(QString("Det%1").arg(i), containWidget);
            // label->setStyleSheet(styleSheet);
            // label->setContentsMargins(9, 0, 0, 0);
            // label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

            // hLayout->addWidget(label);
            // hLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));
            // containWidget->layout()->addItem(hLayout);

            QCustomPlot *customPlot = allocCustomPlot(QString("Det%1").arg(i), spPlotWindow);
            customPlot->setProperty("SelectDetIndex", i-1 );
            //containWidget->layout()->addWidget(customPlot);
            spPlotWindow->addWidget(customPlot/*containWidget*/);
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
        // QLabel* label = new QLabel(tr("符合结果"));
        // label->setStyleSheet(styleSheet);
        // label->setObjectName("labelCoincidenceResult");
        // label->setContentsMargins(9, 0, 0, 0);
        // label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        // containWidget->layout()->addWidget(label);
        QCustomPlot *customPlot = allocCustomPlot("CoincidenceResult", spPlotWindow);
        containWidget->layout()->addWidget(customPlot);
        spPlotWindow->addWidget(containWidget/*customPlot*//*containWidget*/);
        customPlot->installEventFilter(this);

        dispatchAdditionalTipFunction(customPlot);
    }

    spPlotWindow->setStretchFactor(0, 1);
    spPlotWindow->setStretchFactor(1, 1);
    spPlotWindow->setStretchFactor(2, 1);

    this->setLayout(new QVBoxLayout(this));
    this->layout()->setContentsMargins(0, 0, 0, 0);
    this->layout()->addWidget(spPlotWindow);

    switchToCountMode(false);
    switchToLogarithmicMode(false);

    timeAllAction = new QAction(tr("全部时长"), this);
    timeM10Action = new QAction(tr("最近10分钟(默认)"), this);
    timeM5Action = new QAction(tr("最近5分钟"), this);
    timeM3Action = new QAction(tr("最近3分钟"), this);
    timeM1Action = new QAction(tr("最近1分钟"), this);
    connect(timeAllAction, SIGNAL(triggered()), this, SLOT(slotCountRefreshTimelength()));
    connect(timeM10Action, SIGNAL(triggered()), this, SLOT(slotCountRefreshTimelength()));
    connect(timeM5Action, SIGNAL(triggered()), this, SLOT(slotCountRefreshTimelength()));
    connect(timeM3Action, SIGNAL(triggered()), this, SLOT(slotCountRefreshTimelength()));
    connect(timeM1Action, SIGNAL(triggered()), this, SLOT(slotCountRefreshTimelength()));
    QActionGroup *actGrp = new QActionGroup(this);
    actGrp->addAction(timeAllAction);
    actGrp->addAction(timeM10Action);
    actGrp->addAction(timeM5Action);
    actGrp->addAction(timeM3Action);
    actGrp->addAction(timeM1Action);
    actGrp->setExclusive(true);
    QList<QAction*> actions = actGrp->actions();
    for (auto action : actions){
        action->setCheckable(true);
    }
    timeM10Action->setChecked(true);

    allowSelectAreaAction = new QAction(tr("选择拟合区域"), this);
    resetPlotAction = new QAction(tr("还原视图"), this);
    connect(allowSelectAreaAction, &QAction::triggered, this, [=](){
        QWhatsThis::showText(QCursor::pos(), tr("请长按鼠标左键或Ctrl+鼠标左键，在能谱图上框选出符合能窗范围"), this);
        //图像进入区域选择模式
        this->allowAreaSelected = true;

        //隐藏拟合曲线和提示信息框
        // QCPItemText* gaussResultItemText = customPlot->findChild<QCPItemText*>("gaussResultItemText");
        // gaussResultItemText->setText("");
        // getGraph(3)->data()->clear();

        //禁止拖拽
        for (int i=1; i<=2; ++i){
            QCustomPlot *customPlot = this->findChild<QCustomPlot*>(QString("Det%1").arg(i));
            //customPlot->setInteraction(QCP::iRangeDrag, false);

            QCPItemText* gaussResultItemText = customPlot->findChild<QCPItemText*>("gaussResultItemText");
            gaussResultItemText->setText("");

            customPlot->graph(1)->data()->clear();
            customPlot->replot();
        }

        //图像停止刷新
        emit sigPausePlot(true);
        // QTimer::singleShot(30000, this, [=](){
        //     btn->setEnabled(true);
        //     emit sigPausePlot(false);
        // });

        // customPlot->replot();
    });

    connect(resetPlotAction, &QAction::triggered, this, [=](){
        this->setProperty("autoRefreshModel", true);
        if (this->property("isCountMode").toBool()){
            QList<QCustomPlot*> customPlots = this->findChildren<QCustomPlot*>();
            for (auto customPlot:customPlots){
                customPlot->rescaleAxes(true);
            }
        } else {
            for (int i=1; i<=2; ++i){
                QCustomPlot *customPlot = this->findChild<QCustomPlot*>(QString("Det%1").arg(i));
                customPlot->rescaleAxes(true);

                QCPItemText* gaussResultItemText = customPlot->findChild<QCPItemText*>("gaussResultItemText");
                gaussResultItemText->setText("");

                customPlot->graph(1)->data()->clear();
                customPlot->replot();
            }
        }
        emit sigPausePlot(false); //是否暂停图像刷新
        //emit sigAreaSelected();//拟合区域选择取消
    });

    // QVector<double> keys, values;
    // for (int i=0; i<8192; ++i){
    //     keys << i;
    //     values << 0;
    // }
    // QCustomPlot *customPlot = this->findChild<QCustomPlot*>("Det2");
    // customPlot->graph(0)->setData(keys, values);
    // customPlot->replot();
}

void PlotWidget::areaSelectFinished()
{
    this->allowAreaSelected = false;
    this->setProperty("disableAreaSelect", true);
}

void PlotWidget::slotCountRefreshTimelength()
{
    qint32 timeRange = -1;
    QAction* action = qobject_cast<QAction*>(sender());
    if (action->text() == tr("最近10分钟"))
        timeRange = 10*60;
    else if (action->text() == tr("最近5分钟"))
        timeRange = 5*60;
    else if (action->text() == tr("最近3分钟"))
        timeRange = 3*60;
    else if (action->text() == tr("最近1分钟"))
        timeRange = 60;

    this->setProperty("autoRefreshModel", true);
    this->setProperty("refresh-time-range", timeRange);

    QList<QCustomPlot*> customPlots = this->findChildren<QCustomPlot*>();
    for (auto customPlot : customPlots){
        customPlot->rescaleAxes(true);
        customPlot->replot();
    }

    emit sigPausePlot(false); //是否暂停图像刷新
    //emit sigAreaSelected();//拟合区域选择取消
}

void PlotWidget::dispatchAdditionalTipFunction(QCustomPlot *customPlot)
{
    // 跟踪的点
    QCPItemTracer *coordsTipItemTracer = new QCPItemTracer(customPlot);
    coordsTipItemTracer->setObjectName("coordsTipItemTracer");
    coordsTipItemTracer->setStyle(QCPItemTracer::tsCircle);//可以选择设置追踪光标的样式，这个是小十字，还有大十字，圆点等样式
    coordsTipItemTracer->setPen(QPen(Qt::green));//设置tracer的颜色绿色
    //coordsTipItemTracer->setPen(graph->pen());//设置tracer的颜色跟曲线
    coordsTipItemTracer->setBrush(QPen(Qt::green).color());
    coordsTipItemTracer->setSize(6);
    coordsTipItemTracer->setVisible(false);

    QCPItemText *coordsTipItemText = new QCPItemText(customPlot);
    coordsTipItemText->setObjectName("coordsTipItemText");
    coordsTipItemText->setLayer("overlay");
    coordsTipItemText->setClipToAxisRect(false);
    coordsTipItemText->setPadding(QMargins(5, 5, 5, 5));
    coordsTipItemText->position->setParentAnchor(coordsTipItemTracer->position);
    //coordsTipItemText->setFont(QFont("宋体", 10));
    coordsTipItemText->setVisible(false);

    // 箭头
    QCPItemLine *coordsTipItemArrowLine = new QCPItemLine(customPlot);
    coordsTipItemArrowLine->setObjectName("coordsTipItemArrowLine");
    coordsTipItemArrowLine->setLayer("overlay");
    //arrow->setPen(graph->pen());//设置箭头的颜色跟随曲线
    coordsTipItemArrowLine->setPen(QPen(Qt::red));//设置箭头的颜色红色
    coordsTipItemArrowLine->setClipToAxisRect(false);
    coordsTipItemArrowLine->setHead(QCPLineEnding::esSpikeArrow);
    coordsTipItemArrowLine->setVisible(false);

    TracerType mTracerType = DataTracer;
    switch (mTracerType) {
        case XAxisTracer:
        {
            coordsTipItemTracer->position->setTypeX(QCPItemPosition::ptPlotCoords);
            coordsTipItemTracer->position->setTypeY(QCPItemPosition::ptAxisRectRatio);

            coordsTipItemText->setBrush(QBrush(QColor(244, 244, 244, 100)));
            coordsTipItemText->setPen(QPen(Qt::black));
            coordsTipItemText->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);

            coordsTipItemArrowLine->end->setParentAnchor(coordsTipItemTracer->position);
            coordsTipItemArrowLine->start->setParentAnchor(coordsTipItemArrowLine->end);
            coordsTipItemArrowLine->start->setCoords(20, 0);//偏移量
            break;
        }
        case YAxisTracer:
        {
            coordsTipItemTracer->position->setTypeX(QCPItemPosition::ptAxisRectRatio);
            coordsTipItemTracer->position->setTypeY(QCPItemPosition::ptPlotCoords);

            coordsTipItemText->setBrush(QBrush(QColor(244, 244, 244, 100)));
            coordsTipItemText->setPen(QPen(Qt::black));
            coordsTipItemText->setPositionAlignment(Qt::AlignRight|Qt::AlignHCenter);

            coordsTipItemArrowLine->end->setParentAnchor(coordsTipItemTracer->position);
            coordsTipItemArrowLine->start->setParentAnchor(coordsTipItemText->position);
            coordsTipItemArrowLine->start->setCoords(-20, 0);//偏移量
            break;
        }
        case DataTracer:
        {
            coordsTipItemTracer->position->setTypeX(QCPItemPosition::ptPlotCoords);
            coordsTipItemTracer->position->setTypeY(QCPItemPosition::ptPlotCoords);

            coordsTipItemText->setBrush(QBrush(QColor(244, 244, 244, 150)));
            //label->setPen(graph->pen());//边框跟随曲线颜色
            coordsTipItemText->setPen(QPen(Qt::red));//边框红色
            coordsTipItemText->setPositionAlignment(Qt::AlignLeft|Qt::AlignVCenter);

            coordsTipItemArrowLine->end->setParentAnchor(coordsTipItemTracer->position);
            coordsTipItemArrowLine->start->setParentAnchor(coordsTipItemArrowLine->end);
            coordsTipItemArrowLine->start->setCoords(25, 0);
            break;
        }
    }

    // 跟踪的点Y轴线
    QCPItemLine *coordsTipItemYLine = new QCPItemLine(customPlot);
    coordsTipItemYLine->setObjectName("coordsTipItemYLine");
    coordsTipItemYLine->setAntialiased(false);
    coordsTipItemYLine->start->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    coordsTipItemYLine->end->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    coordsTipItemYLine->setPen(QPen(QColor(0,0,255,255), 1, Qt::DotLine));
    coordsTipItemYLine->setSelectedPen(QPen(QColor(0,0,255,255), 1, Qt::DotLine));
    coordsTipItemYLine->setVisible(false);

    // 跟踪的点X轴线
    QCPItemLine *coordsTipItemXLine = new QCPItemLine(customPlot);
    coordsTipItemXLine->setObjectName("coordsTipItemXLine");
    coordsTipItemXLine->setAntialiased(false);
    coordsTipItemXLine->start->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    coordsTipItemXLine->end->setType(QCPItemPosition::ptPlotCoords);//QCPItemPosition::ptAbsolute);
    coordsTipItemXLine->setPen(QPen(QColor(0,0,255,255), 1, Qt::DotLine));
    coordsTipItemXLine->setSelectedPen(QPen(QColor(0,0,255,255), 1, Qt::DotLine));
    coordsTipItemXLine->setVisible(false);
}

void PlotWidget::updateTracerPosition(QCustomPlot* customPlot, double key, double value)
{
    QCPItemTracer *coordsTipItemTracer = customPlot->findChild<QCPItemTracer*>("coordsTipItemTracer");
    QCPItemText *coordsTipItemText = customPlot->findChild<QCPItemText*>("coordsTipItemText");
    QCPItemLine *coordsTipItemArrowLine = customPlot->findChild<QCPItemLine*>("coordsTipItemArrowLine");
    QCPItemLine* coordsTipItemXLine = customPlot->findChild<QCPItemLine*>("coordsTipItemXLine");
    QCPItemLine* coordsTipItemYLine = customPlot->findChild<QCPItemLine*>("coordsTipItemYLine");

    coordsTipItemTracer->setVisible(true);
    coordsTipItemText->setVisible(true);
    coordsTipItemArrowLine->setVisible(true);
    coordsTipItemXLine->setVisible(true);
    coordsTipItemYLine->setVisible(true);

    if (value > customPlot->yAxis->range().upper)
        value = customPlot->yAxis->range().upper;

    TracerType mTracerType = DataTracer;
    switch (mTracerType) {
        case XAxisTracer:
        {
            coordsTipItemTracer->position->setCoords(key, 1);
            coordsTipItemText->position->setCoords(0, 15);
            coordsTipItemArrowLine->start->setCoords(0, 15);
            coordsTipItemArrowLine->end->setCoords(0, 0);

            break;
        }
        case YAxisTracer:
        {
            coordsTipItemTracer->position->setCoords(1, value);
            coordsTipItemText->position->setCoords(-20, 0);
            break;
        }
        case DataTracer:
        {
            coordsTipItemTracer->position->setCoords(key, value);
            coordsTipItemText->position->setCoords(25, 0);
            coordsTipItemText->setText(tr("%1,%2").arg(QString::number(key, 'f', 0)).arg(QString::number(value, 'f', 2)));
            break;
        }
        default:
            break;
    }

    slotShowTracerLine(customPlot, key, value);
}

void PlotWidget::slotShowTracerLine(QCustomPlot* customPlot, double key, double value)
{
    QCPItemLine* coordsTipItemXLine = customPlot->findChild<QCPItemLine*>("coordsTipItemXLine");
    QCPItemLine* coordsTipItemYLine = customPlot->findChild<QCPItemLine*>("coordsTipItemYLine");
    coordsTipItemXLine->start->setCoords(key, customPlot->yAxis->range().lower);
    coordsTipItemXLine->end->setCoords(key, customPlot->yAxis->range().upper);
    coordsTipItemYLine->start->setCoords(customPlot->xAxis->range().lower, value);
    coordsTipItemYLine->end->setCoords(customPlot->xAxis->range().upper, value);
}

void PlotWidget::slotShowTracer(QMouseEvent *event)
{
    //区域选择的时候，不做跟踪处理
    if (this->allowAreaSelected)
        return;

    if (event->button() != Qt::LeftButton)
        return;

    QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(sender());
    QCPItemTracer *coordsTipItemTracer = customPlot->findChild<QCPItemTracer*>("coordsTipItemTracer");
    QCPItemText *coordsTipItemText = customPlot->findChild<QCPItemText*>("coordsTipItemText");
    QCPItemLine *coordsTipItemArrowLine = customPlot->findChild<QCPItemLine*>("coordsTipItemArrowLine");
    QCPItemLine* coordsTipItemXLine = customPlot->findChild<QCPItemLine*>("coordsTipItemXLine");
    QCPItemLine* coordsTipItemYLine = customPlot->findChild<QCPItemLine*>("coordsTipItemYLine");

    double key = customPlot->xAxis->pixelToCoord(event->pos().x());
    QCPGraph *graph = customPlot->graph(0);
    QSharedPointer<QCPGraphDataContainer> data = graph->data();


    QCPDataRange rangeFind(key, key);
    if (graph->data()->dataRange().contains(rangeFind)){
        //const QCPGraphData *ghd = data->at(key);
        QVector<QCPGraphData>::const_iterator ghd = data->findBegin(key, false);
        if (ghd != data->constEnd() && ghd->mainKey()>=0 && ghd->mainValue()>=0){
            updateTracerPosition(customPlot, ghd->mainKey(), ghd->mainValue());
            if (this->property("isCountMode").toBool())
                customPlot->setProperty("tracer-key2", ghd->mainKey());
            else
                customPlot->setProperty("tracer-key", ghd->mainKey());
            customPlot->replot();
            return;
        }
    }

    {
        coordsTipItemTracer->setVisible(false);
        coordsTipItemText->setVisible(false);
        coordsTipItemArrowLine->setVisible(false);
        coordsTipItemXLine->setVisible(false);
        coordsTipItemYLine->setVisible(false);
        if (this->property("isCountMode").toBool())
            customPlot->setProperty("tracer-key2", -1);
        else
            customPlot->setProperty("tracer-key", -1);
        customPlot->replot();
    }
}

void PlotWidget::dispatchAdditionalDragFunction(QCustomPlot *customPlot)
{
    // 文本元素随坐标变动而变动
    QCPItemText* gaussResultItemText = new QCPItemText(customPlot);
    gaussResultItemText->setObjectName("gaussResultItemText");
    gaussResultItemText->setColor(QColor(0,128,128,255));// 字体色
    gaussResultItemText->setPen(QColor(130,130,130,100));// 边框
    gaussResultItemText->setBrush(QBrush(QColor(255,255,225,200)));// 背景色
    gaussResultItemText->setPositionAlignment(Qt::AlignBottom | Qt::AlignLeft);
    gaussResultItemText->position->setType(QCPItemPosition::ptAbsolute);
    gaussResultItemText->setTextAlignment(Qt::AlignLeft);
    gaussResultItemText->setFont(QFont(font().family(), 12));
    gaussResultItemText->setPadding(QMargins(5, 5, 5, 5));
    gaussResultItemText->position->setCoords(55, 25);//X、Y轴坐标值
    gaussResultItemText->setLayer("overlay");
    gaussResultItemText->setText("");
    gaussResultItemText->setVisible(true);

    //创建符合能窗边界线
    auto createStraightLineItem = [=](QCPItemStraightLine** _line){
        QCPItemStraightLine *line = new QCPItemStraightLine(customPlot);
        line->setLayer("overlay");
        line->setPen(QPen(Qt::blue, 1, Qt::SolidLine));
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

    //程序开始启动，还未设置能窗区域范围，所以这里先不显示出来，框选之后才会显示
    itemStraightLineLeft->setVisible(false);
    itemStraightLineRight->setVisible(false);
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
    customPlot->xAxis->rescale(true);
    customPlot->yAxis->rescale(true);
    // 设置刻度范围
//    customPlot->xAxis->setRange(0, 180);
//    customPlot->yAxis->setRange(Y_AXIS_LOWER, 10000);
    customPlot->yAxis->ticker()->setTickCount(5);
    customPlot->xAxis->ticker()->setTickCount(10);

    customPlot->yAxis2->ticker()->setTickCount(5);
    customPlot->xAxis2->ticker()->setTickCount(10);
    // 设置刻度可见
    customPlot->xAxis->setTicks(axisVisible);
    customPlot->xAxis2->setTicks(axisVisible);
    customPlot->yAxis->setTicks(axisVisible);
    customPlot->yAxis2->setTicks(axisVisible);
    // 设置刻度高度
    customPlot->xAxis->setTickLength(13);
    customPlot->yAxis->setTickLength(13);
    customPlot->xAxis->setSubTickLength(4);
    customPlot->yAxis->setSubTickLength(4);

    customPlot->xAxis2->setTickLength(13);
    customPlot->yAxis2->setTickLength(13);
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
    //customPlot->xAxis->setLabel(QObject::tr("时间/S"));
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

    customPlot->xAxis->setRangeLower(0);
    customPlot->yAxis->setRangeLower(0);

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
        curveGraph->setPen(QPen(QBrush(clrGauseCurve), 2, Qt::SolidLine));
        curveGraph->selectionDecorator()->setPen(QPen(clrGauseCurve));
        curveGraph->setLineStyle(QCPGraph::lsLine);
        curveGraph->setVisible(false);
    }

    //connect(customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*, int, QMouseEvent*)), this, SLOT(slotPlotClick(QCPAbstractPlottable*, int, QMouseEvent*)));
    connect(customPlot, SIGNAL(beforeReplot()), this, SLOT(slotBeforeReplot()));
    connect(customPlot, SIGNAL(afterLayout()), this, SLOT(slotBeforeReplot()));

    connect(customPlot->xAxis, SIGNAL(rangeChanged(const QCPRange &)), customPlot->xAxis2, SLOT(setRange(const QCPRange &)));
    connect(customPlot->yAxis, SIGNAL(rangeChanged(const QCPRange &)), customPlot->yAxis2, SLOT(setRange(const QCPRange &)));
    connect(customPlot->xAxis, QOverload<const QCPRange &, const QCPRange &>::of(&QCPAxis::rangeChanged), this, [=](const QCPRange &range, const QCPRange &oldRange){
        if (!this->property("isCountMode").toBool()){
            //能谱模式显示能窗范围
            if (range.lower < 0)
                //customPlot->xAxis->setRangeLower(0);
                customPlot->xAxis->setRange(0, oldRange.size());
            if (range.upper > MAX_SPECTUM)
                //customPlot->xAxis->setRangeUpper(MAX_SPECTUM);
                customPlot->xAxis->setRange(MAX_SPECTUM - oldRange.size(), MAX_SPECTUM);
            if (range.size() < 60)
                customPlot->xAxis->setRange(range.lower, range.lower + 60);//0.01~1000
        } else {
            if (range.lower < 0)
                customPlot->xAxis->setRange(0, oldRange.size());
            if (range.size() < 60)
                customPlot->xAxis->setRange(range.lower, range.lower + 60);//0.01~1000
        }
    });
    connect(customPlot->yAxis, QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this, [=](const QCPRange &range){
        qint64 maxRange = range.upper - range.lower;
        if (this->property("isLogarithmic").toBool()){
            if (maxRange < 1e2){
               customPlot->yAxis->setRange(Y_AXIS_LOWER, range.lower + 1e2);//0.01~1000
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

    // 是否允许X轴自适应缩放
    connect(customPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(slotShowTracer(QMouseEvent*)));
    connect(customPlot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(slotRestorePlot(QMouseEvent*)));
    //connect(customPlot, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(slotRestorePlot(QWheelEvent*)));
    //connect(customPlot, SIGNAL(mouseMove(QMouseEvent*)), this,SLOT(slotShowTracer(QMouseEvent*)));
    connect(customPlot, &QCustomPlot::mouseWheel, this, [=](QWheelEvent*){
        if (this->allowAreaSelected)
            return;

        if (!this->property("isCountMode").toBool())
            return ;

        this->setProperty("autoRefreshModel", false);
        QTimer* timerAutoRestore = this->findChild<QTimer*>("timerAutoRestore");
        if (nullptr == timerAutoRestore){
            timerAutoRestore = new QTimer(this);
            connect(timerAutoRestore, &QTimer::timeout, this, [=](){
                timerAutoRestore->stop();
                this->setProperty("autoRefreshModel", true);
            });
        }

        timerAutoRestore->stop();
        timerAutoRestore->start(10000);//10秒后恢复自动刷新
    });

    return customPlot;
}

void PlotWidget::slotRestorePlot(QMouseEvent* e)
{
    if (this->allowAreaSelected)
        return;

    if (e->button() != Qt::LeftButton)
        return;

    if (!this->property("isCountMode").toBool())
        return ;

    this->setProperty("autoRefreshModel", false);

    QTimer* timerAutoRestore = this->findChild<QTimer*>("timerAutoRestore");
    if (nullptr == timerAutoRestore){
       timerAutoRestore = new QTimer(this);
       connect(timerAutoRestore, &QTimer::timeout, this, [=](){
           timerAutoRestore->stop();
           this->setProperty("autoRefreshModel", true);
       });
    }

    timerAutoRestore->stop();
    timerAutoRestore->start(10000);//10秒后恢复自动刷新
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
                    if (this->property("isCountMode").toBool())
                        return QWidget::eventFilter(watched, event);

                    if (this->allowAreaSelected/* && this->property("allowAreaSelected-plot-name").toString() == customPlot->objectName()*/){
                        this->isPressed = true;
                        this->dragStart = e->pos();
                        this->setProperty("SelectDetIndex", customPlot->property("SelectDetIndex").toUInt());

                        customPlot->setInteraction(QCP::iRangeDrag, false);

                        double key, value;
                        QCPGraph *graph = getGraph(this->property("SelectDetIndex").toUInt());
                        graph->pixelsToCoords(dragStart.x(), dragStart.y(), key, value);
                        QCPItemStraightLine* itemStraightLineLeft = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineLeft");
                        if (itemStraightLineLeft){
                            itemStraightLineLeft->point1->setCoords(key, customPlot->yAxis->range().lower);
                            itemStraightLineLeft->point2->setCoords(key, customPlot->yAxis->range().upper);
                            itemStraightLineLeft->setVisible(true);
                        }
                        QCPItemStraightLine* itemStraightLineRight = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineRight");
                        if (itemStraightLineRight){
                            itemStraightLineRight->point1->setCoords(key, customPlot->yAxis->range().lower);
                            itemStraightLineRight->point2->setCoords(key, customPlot->yAxis->range().upper);
                            itemStraightLineRight->setVisible(true);
                        }

                        customPlot->replot();
                    }
                } else if (e->button() == Qt::RightButton) {// 右键恢复
                    this->allowAreaSelected = false;
                    this->isPressed = false;
                    this->isDragging = false;

                    if (this->property("isCountMode").toBool()){
                        QMenu contextMenu(customPlot);
                        contextMenu.addAction(timeAllAction);
                        contextMenu.addAction(timeM10Action);
                        contextMenu.addAction(timeM5Action);
                        contextMenu.addAction(timeM3Action);
                        contextMenu.addAction(timeM1Action);
                        contextMenu.addSeparator();
                        contextMenu.addAction(resetPlotAction);
                        contextMenu.exec(QCursor::pos());
                    } else {
                        QMenu contextMenu(customPlot);
                        if (!this->property("disableAreaSelect").isValid() || !this->property("disableAreaSelect").toBool())
                            contextMenu.addAction(allowSelectAreaAction);
                        contextMenu.addAction(resetPlotAction);
                        contextMenu.exec(QCursor::pos());
                    }

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
                        if (this->allowAreaSelected){//鼠标框选开始
                            this->isDragging = true;

                            QMutexLocker locker(&mutexRefreshPlot);
                            double key, value;
                            QCPGraph *graph = getGraph(this->property("SelectDetIndex").toUInt());
                            graph->pixelsToCoords(e->pos().x(), e->pos().y(), key, value);
                            QCPItemStraightLine* itemStraightLineRight = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineRight");
                            if (itemStraightLineRight){
                                itemStraightLineRight->point1->setCoords(key, customPlot->yAxis->range().lower);
                                itemStraightLineRight->point2->setCoords(key, customPlot->yAxis->range().upper);
                                itemStraightLineRight->setVisible(true);

                                this->setProperty("area-from-key", key);
                                customPlot->replot();
                            }

                            setCursor(Qt::CrossCursor);
                        } else {
                            setCursor(Qt::SizeAllCursor);
                        }
                    }else {
                        if (this->isDragging) {
                            //拖拽中
                            QMutexLocker locker(&mutexRefreshPlot);
                            double key, value;
                            QCPGraph *graph = getGraph(this->property("SelectDetIndex").toUInt());
                            graph->pixelsToCoords(e->pos().x(), e->pos().y(), key, value);
                            QCPItemStraightLine* itemStraightLineRight = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineRight");
                            if (itemStraightLineRight){
                                itemStraightLineRight->point1->setCoords(key, customPlot->yAxis->range().lower);
                                itemStraightLineRight->point2->setCoords(key, customPlot->yAxis->range().upper);
                                itemStraightLineRight->setVisible(true);

                                this->setProperty("area-to-key", key);
                            }

                            int fcount = 0;
                            std::vector<double> sx;
                            std::vector<double> sy;
                            //框选完毕，将选中的点颜色更新
                            {
                                double key_from = this->property("area-from-key").toDouble(); //dragRectItem->topLeft->key();
                                double key_to = this->property("area-to-key").toDouble(); //dragRectItem->bottomRight->key();
                                double key_temp = key_from;
                                key_from = qMin(key_from, key_to);
                                key_to = qMax(key_temp, key_to);

                                QVector<double> keys, values, curveKeys;
                                QVector<QColor> colors;
                                bool inArea = false;
                                for (int i=0; i<graph->data()->size(); ++i){
                                    if (graph->data()->at(i)->key>=key_from && graph->data()->at(i)->key<=key_to){// && graph->data()->at(i)->value>=value_to) {
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
                                        colors << clrLine;// graph->data()->at(i)->color;
                                    }
                                }

                                graph->setData(keys, values, colors);
                                customPlot->replot();
                            }
                        }
                    }
                } else {
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

                    this->setProperty("ActiveCustomPlotName", customPlot->objectName());
                    if (this->allowAreaSelected){
                        {
                            QMutexLocker locker(&mutexRefreshPlot);

                            QCPGraph *graph = getGraph(this->property("SelectDetIndex").toUInt());
                            int fcount = 0;
                            std::vector<double> sx;
                            std::vector<double> sy;
                            //框选完毕，将选中的点颜色更新
                            {
                                double key_from = this->property("area-from-key").toDouble(); //dragRectItem->topLeft->key();
                                double key_to = this->property("area-to-key").toDouble(); //dragRectItem->bottomRight->key();
                                double key_temp = key_from;
                                key_from = qMin(key_from, key_to);
                                key_to = qMax(key_temp, key_to);

                                QVector<double> keys, values, curveKeys;
                                QVector<QColor> colors;
                                bool inArea = false;
                                for (int i=0; i<graph->data()->size(); ++i){
                                    if (graph->data()->at(i)->key>=key_from && graph->data()->at(i)->key<=key_to){// && graph->data()->at(i)->value>=value_to) {
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
                                        colors << clrLine;// graph->data()->at(i)->color;
                                    }
                                }

                                graph->setData(keys, values, colors);
                                customPlot->replot();

                                //emit sigPausePlot(false);
                                emit sigAreaSelected();
                            }

                            // 高斯拟合
                            if (fcount > 0){
                                //显示拟合曲线
                                double result[3];
                                Gaussian gau;
                                if(gau.setSample(sx, sy, fcount, result) && gau.process())
                                {
                                    if (!std::isnan(result[0]) && !std::isnan(result[1]) && !std::isnan(result[2])){
                                        //计算符合能窗
                                        int leftWindow = (result[1] - result[0] / 2);
                                        if (leftWindow < 0) leftWindow = 0;

                                        int rightWindow = (result[1] + result[0] / 2);
                                        if (rightWindow < 0 || rightWindow > MULTI_CHANNEL)
                                        {
                                            rightWindow = MULTI_CHANNEL;
                                        }
                                        emit sigUpdateMeanValues(customPlot->objectName(), leftWindow, rightWindow);

                                        //显示拟合数据
                                        QCPItemText* gaussResultItemText = customPlot->findChild<QCPItemText*>("gaussResultItemText");
                                        if (gaussResultItemText){
                                            QString info = QString("峰  位: %1\n半高宽: %2\n左能窗: %3\n右能窗: %4")
                                                               .arg(QString::number(result[1], 'f', 0))
                                                               .arg(QString::number(result[0], 'f', 3))
                                                               .arg(QString::number(leftWindow, 10))//十进制输出整数
                                                               .arg(QString::number(rightWindow, 10));

                                            gaussResultItemText->setText(info);
                                            //double key, value;
                                            //graph->pixelsToCoords(e->pos().x(), e->pos().y(), key, value);
                                            //gaussResultItemText->position->setCoords(key, value);//以鼠标所在位置坐标为显示位置
                                            gaussResultItemText->position->setCoords(result[1], result[2]);//以峰值坐标为显示位置
                                            //gaussResultItemText->setVisible(true);
                                            customPlot->replot();
                                        }
                                    }
                                }
                            }
                        }
                    }

                    this->isPressed = false;
                    this->isDragging = false;
                    //this->allowAreaSelected = false;

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
    QCPItemLine* coordsTipItemXLine = customPlot->findChild<QCPItemLine*>("coordsTipItemXLine");
    QCPItemLine* coordsTipItemYLine = customPlot->findChild<QCPItemLine*>("coordsTipItemYLine");
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
        QCPItemLine* flagItemXLine = customPlot->findChild<QCPItemLine*>("flagItemLine");
        if (coordsTipItemXLine){
            coordsTipItemXLine->setProperty("key", ghd->key);
            coordsTipItemXLine->setProperty("value", ghd->value);
            coordsTipItemXLine->setVisible(true);
        }

        customPlot->replot();

        QTimer::singleShot(3000, this, [=](){
            coordsTipItemXLine->setVisible(false);
            coordsTipItemText->setVisible(false);
            customPlot->replot();
        });
    }

    setCursor(Qt::ArrowCursor);
}

void PlotWidget::slotBeforeReplot()
{
    QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(sender());
    QCPItemLine* coordsTipItemXLine = customPlot->findChild<QCPItemLine*>("coordsTipItemXLine");
    QCPItemLine* coordsTipItemYLine = customPlot->findChild<QCPItemLine*>("coordsTipItemYLine");
    if (coordsTipItemXLine && coordsTipItemXLine->visible()){
        QCPGraph *graph = getGraph(this->property("SelectDetIndex").toUInt());
        double key = coordsTipItemXLine->property("key").toUInt();
        double value = coordsTipItemXLine->property("value").toUInt();
        QPointF pixels = graph->coordsToPixels(key, value);
        graph->pixelsToCoords(pixels.x() + 1, pixels.y() - 2, key, value);
        coordsTipItemXLine->start->setCoords(key, value);
        graph->pixelsToCoords(pixels.x() + 1, pixels.y() - 32, key, value);
        coordsTipItemXLine->end->setCoords(key, value);
    }

    QCPItemText* gaussResultItemText = customPlot->findChild<QCPItemText*>("gaussResultItemText");
    if (gaussResultItemText){
        gaussResultItemText->position->setCoords(customPlot->axisRect()->topLeft() + QPointF(50., 100.));
    }

    double key = customPlot->property("tracer-key").toDouble();
    if (this->property("isCountMode").toBool())
        key = customPlot->property("tracer-key2").toDouble();

    QCPGraph *graph = customPlot->graph(0);
    QSharedPointer<QCPGraphDataContainer> data = graph->data();
    QCPDataRange rangeFind(key, key);
    if (!graph->data()->dataRange().contains(rangeFind))
        return;

    //const QCPGraphData *ghd = data->at(key);
    QVector<QCPGraphData>::const_iterator ghd = data->findBegin(key, false);
    if (ghd != data->constEnd() && ghd->mainKey()>=0 && ghd->mainValue()>=0){
        updateTracerPosition(customPlot, ghd->mainKey(), ghd->mainValue());
    }
}

void PlotWidget::slotUpdatePlotNullData(int refreshTime)
{
    QMutexLocker locker(&mutexRefreshPlot);
    QCustomPlot::RefreshPriority refreshPriority = QCustomPlot::rpQueuedReplot;
    if (this->property("isMergeMode").toBool()){
        QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        if (this->property("isCountMode").toBool()){//计数模式
        } else {//能谱
            // {//Det1
            //     double maxEnergy = 0;
            //     QVector<double> keys, values;
            //     QVector<QColor> colors;
            //     for (int i=0; i<MULTI_CHANNEL; ++i){
            //         if (customPlotDet12->graph(0)->data()->size() > 0)
            //             colors << customPlotDet12->graph(0)->data()->at(i)->color;
            //         else
            //             colors << clrLine;
            //     }

            //     for (int i=0; i<MULTI_CHANNEL; ++i){
            //         keys << i+1;
            //         values << r1.spectrum[0][i];
            //         energyValues[0][i] = r1.spectrum[0][i];
            //         maxEnergy = qMax(maxEnergy, values[i]);
            //     }

            //     customPlotDet12->graph(0)->setData(keys, values, colors);
            //     if (maxEnergy > customPlotDet12->yAxis->range().upper * RANGE_SCARRE_UPPER){
            //         customPlotDet12->yAxis->setRange(customPlotDet12->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
            //     }
            // }

            // {//Det2
            //     double maxEnergy = 0;
            //     QVector<double> keys, values;
            //     QVector<QColor> colors;
            //     for (int i=0; i<MULTI_CHANNEL; ++i){
            //         if (customPlotDet12->graph(1)->data()->size() > 0)
            //             colors << customPlotDet12->graph(1)->data()->at(i)->color;
            //         else
            //             colors << clrLine;
            //     }

            //     for (int i=0; i<MULTI_CHANNEL; ++i){
            //         keys << i+1;
            //         values << r1.spectrum[1][i];
            //         energyValues[1][i] = r1.spectrum[1][i];
            //         maxEnergy = qMax(maxEnergy, values[i]);
            //     }

            //     customPlotDet12->graph(1)->setData(keys, values, colors);
            //     if (maxEnergy > customPlotDet12->yAxis->range().upper * RANGE_SCARRE_UPPER){
            //         customPlotDet12->yAxis->setRange(customPlotDet12->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
            //     }

            //     customPlotDet12->replot(refreshPriority);
            // }
        }
    } else {
        QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
        QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        if (this->property("isCountMode").toBool()){//计数模式
        } else {//能谱
            {//Det1
                double maxEnergy = 0;
                QVector<double> keys, values;
                QVector<QColor> colors;
                for (int i=0; i<MULTI_CHANNEL; ++i){
                    // if (customPlotDet1->graph(0)->data()->size() > 0)
                    //     colors << customPlotDet1->graph(0)->data()->at(i)->color;
                    // else
                    colors << clrLine;
                }

                for (int i=0; i<MULTI_CHANNEL; ++i){
                    keys << i+1;
                    values << 0;
                    energyValues[0][i] = 0;
                    maxEnergy = qMax(maxEnergy, values[i]);
                }

                customPlotDet1->graph(0)->setData(keys, values, colors);
                if (maxEnergy > customPlotDet1->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotDet1->yAxis->setRange(customPlotDet1->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
                    customPlotDet1->yAxis2->setRange(customPlotDet1->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
                }

                customPlotDet1->replot(refreshPriority);
            }

            {//Det2
                double maxEnergy = 0;
                QVector<double> keys, values;
                QVector<QColor> colors;
                for (int i=0; i<MULTI_CHANNEL; ++i){
                    // if (customPlotDet2->graph(0)->data()->size() > 0)
                    //     colors << customPlotDet2->graph(0)->data()->at(i)->color;
                    // else
                    colors << clrLine;
                }

                for (int i=0; i<MULTI_CHANNEL; ++i){
                    keys << i+1;
                    values << 0;
                    energyValues[1][i] = 0;
                    maxEnergy = qMax(maxEnergy, values[i]);
                }

                customPlotDet2->graph(0)->setData(keys, values, colors);
                if (maxEnergy > customPlotDet2->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotDet2->yAxis->setRange(customPlotDet2->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
                    customPlotDet2->yAxis2->setRange(customPlotDet2->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
                }

                customPlotDet2->replot(refreshPriority);
            }
        }
    }
}

#include <QtMath>
#include <math.h>
void PlotWidget::slotUpdatePlotDatas(SingleSpectrum r1, vector<CoincidenceResult> r3, int refreshTime)
{
    QMutexLocker locker(&mutexRefreshPlot);
    QCustomPlot::RefreshPriority refreshPriority = QCustomPlot::rpQueuedReplot;
    if (this->property("isMergeMode").toBool()){
        QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        if (this->property("isCountMode").toBool()){//计数模式
            {//Det1
                double maxPoint = 0.;
                QVector<double> keys, values;
                QVector<QColor> colors;
                for (size_t i=0; i<r3.size(); ++i){
                    uint32_t key = (i+1)*refreshTime;
                    keys << key;
                    values << r3[i].CountRate1;
                    colors << clrLine;
                    maxPoint = qMax(maxPoint, (double)r3[i].CountRate1);
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
                for (size_t i=0; i<r3.size(); ++i){
                    uint32_t key = (i+1)*refreshTime;
                    keys << key;
                    values << r3[i].CountRate2;
                    colors << clrLine2;
                    maxPoint = qMax(maxPoint, (double)r3[i].CountRate2);
                }

                if (customPlotDet12->graph(1)->visible()){
                    customPlotDet12->graph(1)->setData(keys, values, colors);
                    if (maxPoint > customPlotDet12->yAxis->range().upper * RANGE_SCARRE_UPPER){
                        customPlotDet12->yAxis->setRange(customPlotDet12->yAxis->range().lower, maxPoint / RANGE_SCARRE_LOWER);
                    }
                }

                if (this->property("autoRefreshModel").toBool()){
                    qint32 timeRange = this->property("refresh-time-range").toInt();
                    if (keys.back() > customPlotDet12->xAxis->range().upper){
                        if (timeRange == -1)
                            customPlotDet12->xAxis->setRange(0, keys.back());
                        else if (keys.back() > timeRange)
                            customPlotDet12->xAxis->setRange(keys.back() - timeRange, keys.back());
                        else
                            customPlotDet12->xAxis->setRange(0, keys.back());
                    }
                }

                customPlotDet12->replot(refreshPriority);
            }

            {//符合结果
                QVector<double> keys, values;
                QVector<QColor> colors;
                double maxCount = 0;

                for (size_t i=0; i<r3.size(); ++i){
                    uint32_t key = (i+1)*refreshTime;
                    keys << key;
                    values << r3[i].ConCount_single;
                    colors << clrLine;
                    maxCount = qMax(maxCount, (double)r3[i].ConCount_single);
                }

                if (maxCount > customPlotCoincidenceResult->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotCoincidenceResult->yAxis->setRange(customPlotCoincidenceResult->yAxis->range().lower, maxCount / RANGE_SCARRE_LOWER);
                }
                customPlotCoincidenceResult->graph(0)->setData(keys, values, colors);

                if (this->property("autoRefreshModel").toBool()){
                    qint32 timeRange = this->property("refresh-time-range").toInt();
                    if (keys.back() > customPlotCoincidenceResult->xAxis->range().upper){
                        if (timeRange == -1)
                            customPlotCoincidenceResult->xAxis->setRange(0, keys.back());
                        else if (keys.back() > timeRange)
                            customPlotCoincidenceResult->xAxis->setRange(keys.back() - timeRange, keys.back());
                        else
                            customPlotCoincidenceResult->xAxis->setRange(0, keys.back());
                    }
                }
                customPlotCoincidenceResult->replot(refreshPriority);
            }

        } else {//能谱
            {//Det1
                double maxEnergy = 0;
                QVector<double> keys, values;
                QVector<QColor> colors;            
                for (int i=0; i<MULTI_CHANNEL; ++i){
                    if (customPlotDet12->graph(0)->data()->size() > 0)
                        colors << customPlotDet12->graph(0)->data()->at(i)->color;
                    else
                        colors << clrLine;
                }

                for (int i=0; i<MULTI_CHANNEL; ++i){
                    keys << i+1;
                    values << r1.spectrum[0][i];
                    energyValues[0][i] = r1.spectrum[0][i];
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
                for (int i=0; i<MULTI_CHANNEL; ++i){
                    if (customPlotDet12->graph(1)->data()->size() > 0)
                        colors << customPlotDet12->graph(1)->data()->at(i)->color;
                    else
                        colors << clrLine;
                }

                for (int i=0; i<MULTI_CHANNEL; ++i){
                    keys << i+1;
                    values << r1.spectrum[1][i];
                    energyValues[1][i] = r1.spectrum[1][i];
                    maxEnergy = qMax(maxEnergy, values[i]);
                }

                customPlotDet12->graph(1)->setData(keys, values, colors);
                if (maxEnergy > customPlotDet12->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotDet12->yAxis->setRange(customPlotDet12->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
                }

                customPlotDet12->replot(refreshPriority);
            }
        }
    } else {
        QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
        QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        if (this->property("isCountMode").toBool()){//计数模式
            {//Det1
                double maxValue = 0.;
                QVector<double> keys, values;
                QVector<QColor> colors;

                for (size_t i=0; i<r3.size(); ++i){
                    uint32_t key = (i+1)*refreshTime;
                    keys << key;
                    values << r3[i].CountRate1;
                    colors << clrLine;
                    maxValue = qMax(maxValue, (double)r3[i].CountRate1);
                }

                customPlotDet1->graph(0)->setData(keys, values, colors);
                if (maxValue > customPlotDet1->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotDet1->yAxis->setRange(customPlotDet1->yAxis->range().lower, maxValue / RANGE_SCARRE_LOWER);
                    customPlotDet1->yAxis2->setRange(customPlotDet1->yAxis->range().lower, maxValue / RANGE_SCARRE_LOWER);//因为这里是子线程调用的，所以联动的槽函数（722行）不会自动触发，需要手动设置
                } else if (maxValue < 10){
                    customPlotDet1->yAxis->setRange(0, 10);
                    customPlotDet1->yAxis2->setRange(0, 10);
                }

                if (this->property("autoRefreshModel").toBool()){
                    qint32 timeRange = this->property("refresh-time-range").toInt();
                    if (keys.back() > customPlotDet1->xAxis->range().upper){
                        if (timeRange == -1){
                            customPlotDet1->xAxis->setRange(0, keys.back());
                            customPlotDet1->xAxis2->setRange(0, keys.back());
                        }
                        else if (keys.back() > timeRange){
                            customPlotDet1->xAxis->setRange(keys.back() - timeRange, keys.back());
                            customPlotDet1->xAxis2->setRange(keys.back() - timeRange, keys.back());
                        }
                        else{
                            customPlotDet1->xAxis->setRange(0, keys.back());
                            customPlotDet1->xAxis2->setRange(0, keys.back());
                        }
                    }
                }

                customPlotDet1->replot(refreshPriority);
            }

            {//Det2
                double maxValue = 0.;
                QVector<double> keys, values;
                QVector<QColor> colors;

                for (size_t i=0; i<r3.size(); ++i){
                    uint32_t key = (i+1)*refreshTime;
                    keys << key;
                    values << r3[i].CountRate2;
                    colors << clrLine;
                    maxValue = qMax(maxValue, (double)r3[i].CountRate2);
                }

                customPlotDet2->graph(0)->setData(keys, values, colors);
                if (maxValue > customPlotDet2->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotDet2->yAxis->setRange(customPlotDet2->yAxis->range().lower, maxValue / RANGE_SCARRE_LOWER);
                    customPlotDet2->yAxis2->setRange(customPlotDet2->yAxis->range().lower, maxValue / RANGE_SCARRE_LOWER);
                }
                else if (maxValue < 10){
                    customPlotDet2->yAxis->setRange(0, 10);
                    customPlotDet2->yAxis2->setRange(0, 10);
                }


                if (this->property("autoRefreshModel").toBool()){
                    qint32 timeRange = this->property("refresh-time-range").toInt();
                    if (keys.back() > customPlotDet2->xAxis->range().upper){
                        if (timeRange == -1){
                            customPlotDet2->xAxis->setRange(0, keys.back());
                            customPlotDet2->xAxis2->setRange(0, keys.back());
                        }
                        else if (keys.back() > timeRange){
                            customPlotDet2->xAxis->setRange(keys.back() - timeRange, keys.back());
                            customPlotDet2->xAxis2->setRange(keys.back() - timeRange, keys.back());
                        }
                        else{
                            customPlotDet2->xAxis->setRange(0, keys.back());
                            customPlotDet2->xAxis2->setRange(0, keys.back());
                        }
                    }
                }

                customPlotDet2->replot(refreshPriority);
            }

            {//符合结果
                QVector<double> keys, values;
                QVector<QColor> colors;
                double maxValue = 0;

                for (size_t i=0; i<r3.size(); ++i){
                    uint32_t key = (i+1)*refreshTime;
                    keys << key;
                    values << r3[i].ConCount_single;
                    colors << clrLine;
                    maxValue = qMax(maxValue, (double)r3[i].ConCount_single);
                }

                if (maxValue > customPlotCoincidenceResult->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotCoincidenceResult->yAxis->setRange(customPlotCoincidenceResult->yAxis->range().lower, maxValue / RANGE_SCARRE_LOWER);
                    customPlotCoincidenceResult->yAxis2->setRange(customPlotCoincidenceResult->yAxis->range().lower, maxValue / RANGE_SCARRE_LOWER);
                } else if (maxValue < 10){
                    customPlotCoincidenceResult->yAxis->setRange(0, 10);
                    customPlotCoincidenceResult->yAxis2->setRange(0, 10);
                }
                customPlotCoincidenceResult->graph(0)->setData(keys, values, colors);

                if (this->property("autoRefreshModel").toBool()){
                    qint32 timeRange = this->property("refresh-time-range").toInt();
                    if (keys.back() > customPlotCoincidenceResult->xAxis->range().upper){
                        if (timeRange == -1){
                            customPlotCoincidenceResult->xAxis->setRange(0, keys.back());
                            customPlotCoincidenceResult->xAxis2->setRange(0, keys.back());
                        }
                        else if (keys.back() > timeRange){
                            customPlotCoincidenceResult->xAxis->setRange(keys.back() - timeRange, keys.back());
                            customPlotCoincidenceResult->xAxis2->setRange(0, keys.back());
                        }
                        else{
                            customPlotCoincidenceResult->xAxis->setRange(0, keys.back());
                            customPlotCoincidenceResult->xAxis2->setRange(0, keys.back());
                        }
                    }
                }

                customPlotCoincidenceResult->replot(refreshPriority);
            }
        } else {//能谱
            {//Det1
                double maxEnergy = 0;
                QVector<double> keys, values;
                QVector<QColor> colors;
                for (int i=0; i<MULTI_CHANNEL; ++i){
                    // if (customPlotDet1->graph(0)->data()->size() > 0)
                    //     colors << customPlotDet1->graph(0)->data()->at(i)->color;
                    // else
                        colors << clrLine;
                }

                for (int i=0; i<MULTI_CHANNEL; ++i){
                    keys << i+1;
                    values << r1.spectrum[0][i];
                    energyValues[0][i] = r1.spectrum[0][i];
                    maxEnergy = qMax(maxEnergy, values[i]);
                }

                customPlotDet1->graph(0)->setData(keys, values, colors);
                if (maxEnergy > customPlotDet1->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotDet1->yAxis->setRange(customPlotDet1->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
                    customPlotDet1->yAxis2->setRange(customPlotDet1->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
                }

                customPlotDet1->replot(refreshPriority);
            }

            {//Det2
                double maxEnergy = 0;
                QVector<double> keys, values;
                QVector<QColor> colors;
                for (int i=0; i<MULTI_CHANNEL; ++i){
                    // if (customPlotDet2->graph(0)->data()->size() > 0)
                    //     colors << customPlotDet2->graph(0)->data()->at(i)->color;
                    // else
                        colors << clrLine;
                }

                for (int i=0; i<MULTI_CHANNEL; ++i){
                    keys << i+1;
                    values << r1.spectrum[1][i];
                    energyValues[1][i] = r1.spectrum[1][i];
                    maxEnergy = qMax(maxEnergy, values[i]);
                }

                customPlotDet2->graph(0)->setData(keys, values, colors);
                if (maxEnergy > customPlotDet2->yAxis->range().upper * RANGE_SCARRE_UPPER){
                    customPlotDet2->yAxis->setRange(customPlotDet2->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
                    customPlotDet2->yAxis2->setRange(customPlotDet2->yAxis->range().lower, maxEnergy / RANGE_SCARRE_LOWER);
                }

                customPlotDet2->replot(refreshPriority);
            }
        }
    }
}

void PlotWidget::slotStart()
{
    //当第一次数据过来，将区域选择按钮设为可用
    this->setProperty("disableAreaSelect", false);

    QCustomPlot::RefreshPriority refreshPriority = QCustomPlot::rpQueuedReplot;
    QList<QCustomPlot*> customPlots = this->findChildren<QCustomPlot*>();
    for (auto customPlot : customPlots){
        customPlot->rescaleAxes(true);
        customPlot->setProperty("tracer-key", -1);//跟踪点坐标值为0
        customPlot->setProperty("tracer-key2", -1);//跟踪点坐标值为0

        QCPItemTracer *coordsTipItemTracer = customPlot->findChild<QCPItemTracer*>("coordsTipItemTracer");
        QCPItemText *coordsTipItemText = customPlot->findChild<QCPItemText*>("coordsTipItemText");
        QCPItemLine *coordsTipItemArrowLine = customPlot->findChild<QCPItemLine*>("coordsTipItemArrowLine");
        QCPItemLine* coordsTipItemXLine = customPlot->findChild<QCPItemLine*>("coordsTipItemXLine");
        QCPItemLine* coordsTipItemYLine = customPlot->findChild<QCPItemLine*>("coordsTipItemYLine");
        QCPItemText* gaussResultItemText = customPlot->findChild<QCPItemText*>("gaussResultItemText");
        coordsTipItemTracer->setVisible(false);
        coordsTipItemText->setVisible(false);
        coordsTipItemArrowLine->setVisible(false);
        coordsTipItemXLine->setVisible(false);
        coordsTipItemYLine->setVisible(false);
        if (gaussResultItemText){
            gaussResultItemText->setText("");
            gaussResultItemText->setVisible(false);
        }
        if (customPlot->graphCount() > 1)
            customPlot->graph(1)->data()->clear();

        customPlot->rescaleAxes(true);
        customPlot->replot(refreshPriority);
    }

    this->slotResetPlot();
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

        customPlotDet12->xAxis->rescale(true);
        customPlotDet12->yAxis->rescale(true);
        customPlotCoincidenceResult->xAxis->rescale(true);
        customPlotCoincidenceResult->yAxis->rescale(true);
    } else {
        QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
        QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        customPlotDet1->replot();
        customPlotDet2->replot();
        customPlotCoincidenceResult->replot();

        customPlotDet1->xAxis->rescale(true);
        customPlotDet1->yAxis->rescale(true);
        customPlotDet2->xAxis->rescale(true);
        customPlotDet2->yAxis->rescale(true);
        customPlotCoincidenceResult->xAxis->rescale(true);
        customPlotCoincidenceResult->yAxis->rescale(true);
    }
}

void PlotWidget::switchToCountMode(bool isCountMode)
{
    QMutexLocker locker(&mutexRefreshPlot);
    this->setProperty("isCountMode", isCountMode);
    QWidget *containWidgetCoincidenceResult = this->findChild<QWidget*>("containWidgetCoincidenceResult");

    getGraph(0)->data()->clear();
    getGraph(1)->data()->clear();
    getGraph(2)->data()->clear();
    getGraph(3)->data()->clear();

    if (this->property("isMergeMode").toBool()){
        QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
        QCustomPlot* customPlotCoincidenceResult = this->findChild<QCustomPlot*>("CoincidenceResult");

        if (this->property("isCountMode").toBool()){
            getGraph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 5));
            getGraph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 5));

            customPlotDet12->xAxis->setLabel(tr("时间/s"));
            customPlotDet12->yAxis->setLabel(tr("计数率/cps"));

            customPlotCoincidenceResult->xAxis->setLabel(tr("时间/s"));
            customPlotCoincidenceResult->yAxis->setLabel(tr("计数率/cps"));

            //高斯曲线隐藏
            getGraph(3)->setVisible(false);

            //符合结果隐藏
            containWidgetCoincidenceResult->setVisible(true);
        } else {
            getGraph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 3));
            getGraph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 3));

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

        if (this->property("isCountMode").toBool()){
            getGraph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 5));
            getGraph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 5));

            customPlotDet1->xAxis->setLabel(tr(""));
            customPlotDet1->yAxis->setLabel(tr("Det-1 计数率/cps"));
            customPlotDet2->xAxis->setLabel(tr(""));
            customPlotDet2->yAxis->setLabel(tr("Det-2 计数率/cps"));

            customPlotCoincidenceResult->xAxis->setLabel(tr("时间/s"));
            customPlotCoincidenceResult->yAxis->setLabel(tr("符合结果 计数率/cps"));

            customPlotDet1->graph(1)->setVisible(false);
            customPlotDet2->graph(1)->setVisible(false);

            customPlotDet1->xAxis->setRange(0, COUNT_X_AXIS_LOWER[0]);
            customPlotDet2->xAxis->setRange(0, COUNT_X_AXIS_LOWER[0]);

            QList<QCustomPlot*> customPlots = this->findChildren<QCustomPlot*>();
            for (auto customPlot : customPlots){
                QCPItemStraightLine* itemStraightLineLeft = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineLeft");
                QCPItemStraightLine* itemStraightLineRight = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineRight");
                if (itemStraightLineLeft) itemStraightLineLeft->setVisible(false);
                if (itemStraightLineRight) itemStraightLineRight->setVisible(false);

                //隐藏峰值信息
                QCPItemText* gaussResultItemText = customPlot->findChild<QCPItemText*>("gaussResultItemText");
                if (gaussResultItemText) gaussResultItemText->setVisible(false);

                //隐藏高斯曲线
                if (customPlot->graphCount() >= 2)
                    customPlot->graph(1)->setVisible(false);
            }

            containWidgetCoincidenceResult->setVisible(true);
        } else {
            getGraph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 3));
            getGraph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 3));

            customPlotDet1->xAxis->setLabel(tr(""));
            customPlotDet1->yAxis->setLabel(tr("Det-1 计数"));
            customPlotDet2->xAxis->setLabel(tr("道址"));
            customPlotDet2->yAxis->setLabel(tr("Det-2 计数"));

            customPlotDet1->graph(1)->setVisible(true);
            customPlotDet2->graph(1)->setVisible(true);

            customPlotDet1->xAxis->setRange(0, MAX_SPECTUM);
            customPlotDet2->xAxis->setRange(0,MAX_SPECTUM);

            QList<QCustomPlot*> customPlots = this->findChildren<QCustomPlot*>();
            for (auto customPlot : customPlots){
                QCPItemStraightLine* itemStraightLineLeft = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineLeft");
                QCPItemStraightLine* itemStraightLineRight = customPlot->findChild<QCPItemStraightLine*>("itemStraightLineRight");
                if (itemStraightLineLeft) itemStraightLineLeft->setVisible(true);
                if (itemStraightLineRight) itemStraightLineRight->setVisible(true);

                //显示峰值信息
                bool visible = this->property("showGaussInfo").toBool();
                QCPItemText* gaussResultItemText = customPlot->findChild<QCPItemText*>("gaussResultItemText");
                if (gaussResultItemText) gaussResultItemText->setVisible(visible);

                //显示高斯曲线
                if (customPlot->graphCount() >= 2)
                    customPlot->graph(1)->setVisible(visible);
            }

            containWidgetCoincidenceResult->setVisible(false);
        }

        resetAxisCoords();

        customPlotDet1->replot();
        customPlotDet2->replot();
        customPlotCoincidenceResult->replot();
    }
}

void PlotWidget::switchToLogarithmicMode(bool isLogarithmic)
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
                customPlot->yAxis->setRange(0, 1e3);
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
                customPlot->yAxis->setRange(0, 1e3);
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

    //QCPItemRect* dragRectItem = customPlot->findChild<QCPItemRect*>("dragRectItem");//鼠标拖拽框
    //if (dragRectItem)
    {
        double key_from = this->property("area-from-key").toDouble(); //dragRectItem->topLeft->key();
        double key_to = this->property("area-to-key").toDouble(); //dragRectItem->bottomRight->key();
        double key_temp = key_from;
        key_from = qMin(key_from, key_to);
        key_to = qMax(key_temp, key_to);

        // double value_from = dragRectItem->topLeft->value();
        // double value_to = dragRectItem->bottomRight->value();
        // double value_temp = value_from;
        // value_from = qMin(value_from, value_to);
        // value_to = qMax(value_temp, value_to);

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
            Gaussian gau;
            if(gau.setSample(sx, sy, fcount, result) && gau.process())
            {
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
}

void PlotWidget::slotUpdateEnTimeWidth(unsigned short* timeWidth)
{
    if (this->property("isCountMode").toBool())
        return;

    if (this->property("isMergeMode").toBool()){
        QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
        QCPItemStraightLine* itemStraightLineLeft = customPlotDet12->findChild<QCPItemStraightLine*>("itemStraightLineLeft");
        if (itemStraightLineLeft){
            itemStraightLineLeft->point1->setCoords(timeWidth[0], customPlotDet12->yAxis->range().lower);
            itemStraightLineLeft->point2->setCoords(timeWidth[0], customPlotDet12->yAxis->range().upper);
            itemStraightLineLeft->setVisible(true);
        }
        QCPItemStraightLine* itemStraightLineRight = customPlotDet12->findChild<QCPItemStraightLine*>("itemStraightLineRight");
        if (itemStraightLineRight){
            itemStraightLineRight->point1->setCoords(timeWidth[1], customPlotDet12->yAxis->range().lower);
            itemStraightLineRight->point2->setCoords(timeWidth[1], customPlotDet12->yAxis->range().upper);
            itemStraightLineRight->setVisible(true);
        }

        customPlotDet12->replot(QCustomPlot::rpQueuedRefresh);
    } else {
        QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
        QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");

        {
            QCPItemStraightLine* itemStraightLineLeft = customPlotDet1->findChild<QCPItemStraightLine*>("itemStraightLineLeft");
            if (itemStraightLineLeft){
                itemStraightLineLeft->point1->setCoords(timeWidth[0], customPlotDet1->yAxis->range().lower);
                itemStraightLineLeft->point2->setCoords(timeWidth[0], customPlotDet1->yAxis->range().upper);
                itemStraightLineLeft->setVisible(true);
            }
            QCPItemStraightLine* itemStraightLineRight = customPlotDet1->findChild<QCPItemStraightLine*>("itemStraightLineRight");
            if (itemStraightLineRight){
                itemStraightLineRight->point1->setCoords(timeWidth[1], customPlotDet2->yAxis->range().lower);
                itemStraightLineRight->point2->setCoords(timeWidth[1], customPlotDet2->yAxis->range().upper);
                itemStraightLineRight->setVisible(true);
            }
        }
        {
            QCPItemStraightLine* itemStraightLineLeft = customPlotDet2->findChild<QCPItemStraightLine*>("itemStraightLineLeft");
            if (itemStraightLineLeft){
                itemStraightLineLeft->point1->setCoords(timeWidth[2], customPlotDet2->yAxis->range().lower);
                itemStraightLineLeft->point2->setCoords(timeWidth[2], customPlotDet2->yAxis->range().upper);
                itemStraightLineLeft->setVisible(true);
            }
            QCPItemStraightLine* itemStraightLineRight = customPlotDet2->findChild<QCPItemStraightLine*>("itemStraightLineRight");
            if (itemStraightLineRight){
                itemStraightLineRight->point1->setCoords(timeWidth[3], customPlotDet2->yAxis->range().lower);
                itemStraightLineRight->point2->setCoords(timeWidth[3], customPlotDet2->yAxis->range().upper);
                itemStraightLineRight->setVisible(true);
            }
        }

        customPlotDet1->replot(QCustomPlot::rpQueuedRefresh);
        customPlotDet2->replot(QCustomPlot::rpQueuedRefresh);
    }
}

void PlotWidget::slotShowGaussInfor(bool visible)
{
    this->setProperty("showGaussInfo", visible);
    if (this->property("isMergeMode").toBool()){
        QCustomPlot* customPlotDet12 = this->findChild<QCustomPlot*>("Det12");
        QCPItemText* gaussResultItemText = customPlotDet12->findChild<QCPItemText*>("gaussResultItemText");
        gaussResultItemText->setVisible(visible);
    } else {
        QCustomPlot* customPlotDet1 = this->findChild<QCustomPlot*>("Det1");
        QCustomPlot* customPlotDet2 = this->findChild<QCustomPlot*>("Det2");
        QCPItemText* gaussResultItemText = customPlotDet1->findChild<QCPItemText*>("gaussResultItemText");
        gaussResultItemText->setVisible(visible);        
        customPlotDet1->replot(QCustomPlot::rpQueuedRefresh);

        gaussResultItemText = customPlotDet2->findChild<QCPItemText*>("gaussResultItemText");
        gaussResultItemText->setVisible(visible);
        customPlotDet2->replot(QCustomPlot::rpQueuedRefresh);
    }
}
