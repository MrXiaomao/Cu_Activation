#include "dataanalysiswidget.h"
#include "ui_dataanalysiswidget.h"
#include "plotwidget.h"

DataAnalysisWidget::DataAnalysisWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataAnalysisWidget)
{
    ui->setupUi(this);

    initCustomPlot();
}

DataAnalysisWidget::~DataAnalysisWidget()
{
    delete ui;
}

void DataAnalysisWidget::initCustomPlot()
{
    //this->setDockNestingEnabled(true);

    // Det1
    PlotWidget *customPlotWidget_Det1 = new PlotWidget(ui->widget_plot);
    customPlotWidget_Det1->setName(tr("Det1"));
    customPlotWidget_Det1->installEventFilter(this);
    customPlotWidget_Det1->show();

    // Det2
    PlotWidget *customPlotWidget_Det2 = new PlotWidget(ui->widget_plot);
    customPlotWidget_Det2->setName(tr("Det2"));
    customPlotWidget_Det2->installEventFilter(this);
    customPlotWidget_Det2->show();

    // result
    PlotWidget *customPlotWidget_result = new PlotWidget(ui->widget_plot);
    customPlotWidget_result->setName(tr("符合结果"));
    customPlotWidget_result->installEventFilter(this);
    customPlotWidget_result->show();

    ui->widget_plot->layout()->addWidget(customPlotWidget_Det1);
    ui->widget_plot->layout()->addWidget(customPlotWidget_Det2);
    ui->widget_plot->layout()->addWidget(customPlotWidget_result);

    connect(customPlotWidget_Det1, SIGNAL(sigMouseDoubleClickEvent()), this, SLOT(slotPlowWidgetDoubleClickEvent()));
    connect(customPlotWidget_Det2, SIGNAL(sigMouseDoubleClickEvent()), this, SLOT(slotPlowWidgetDoubleClickEvent()));
    connect(customPlotWidget_result, SIGNAL(sigMouseDoubleClickEvent()), this, SLOT(slotPlowWidgetDoubleClickEvent()));
}

void DataAnalysisWidget::slotPlowWidgetDoubleClickEvent()
{
    PlotWidget* plotwidget = qobject_cast<PlotWidget*>(sender());
    bool isFullRect = false;
    if (plotwidget->property("isFullRect").isValid())
        isFullRect = plotwidget->property("isFullRect").toBool();

    isFullRect = !isFullRect;
    plotwidget->setProperty("isFullRect", isFullRect);
    QList<PlotWidget*> plotWidgets = this->findChildren<PlotWidget*>();
    for (auto a : plotWidgets){
        bool _isFullRect = false;
        if (a->property("isFullRect").isValid()){
            _isFullRect = a->property("isFullRect").toBool();
        }

        if (isFullRect){
            if (a != plotwidget)
                a->hide();
            else
                a->show();
        } else {
            a->show();
        }
    };

    plotwidget->show();
}
