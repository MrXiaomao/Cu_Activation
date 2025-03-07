#include "dataanalysiswidget.h"
#include "ui_dataanalysiswidget.h"
#include "plotwidget.h"

DataAnalysisWidget::DataAnalysisWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataAnalysisWidget)
{
    ui->setupUi(this);

    initCustomPlot();

    ui->label_tag->installEventFilter(this);
    ui->label_tag_2->installEventFilter(this);
    ui->label_tag_3->installEventFilter(this);
    ui->label_tag_->installEventFilter(this);
    ui->label_tag_2_->installEventFilter(this);
    ui->label_tag_3_->installEventFilter(this);

    ui->label_tag->setBuddy(ui->widget_tag);
    ui->label_tag_->setBuddy(ui->widget_tag);
    ui->label_tag_2->setBuddy(ui->widget_tag_2);
    ui->label_tag_2_->setBuddy(ui->widget_tag_2);
    ui->label_tag_3->setBuddy(ui->widget_tag_3);
    ui->label_tag_3_->setBuddy(ui->widget_tag_3);

    //ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 平分
}

DataAnalysisWidget::~DataAnalysisWidget()
{
    delete ui;
}

#include <QMouseEvent>
bool DataAnalysisWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != this){
        if (event->type() == QEvent::MouseButtonPress){
            if (watched->inherits("QLabel")){
                QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
                if (e->button() == Qt::LeftButton) {
                    QLabel* label = qobject_cast<QLabel*>(watched);
                    bool toggled = false;
                    if (label->property("toggled").isValid())
                        toggled = label->property("toggled").toBool();

                    toggled = !toggled;
                    label->setProperty("toggled", toggled);
                    if (toggled){
                        label->setPixmap(QPixmap(":/resource/arrow-drop-up-x16.png"));
                        QWidget * w = label->buddy();
                        w->hide();
                    } else {
                        label->setPixmap(QPixmap(":/resource/arrow-drop-down-x16.png"));
                        QWidget * w = label->buddy();
                        w->show();
                    }
                }
            }
        }
    }

    return QWidget::eventFilter(watched, event);
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
