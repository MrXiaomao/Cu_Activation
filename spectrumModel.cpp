#include "spectrumModel.h"
#include "ui_spectrumModel.h"
#include "plotwidget.h"

SpectrumModel::SpectrumModel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SpectrumModel)
    , measuring(false)
{
    ui->setupUi(this);

    initCustomPlot();
}

SpectrumModel::~SpectrumModel()
{
    delete ui;
}

void SpectrumModel::on_pushButton_start_clicked()
{
    QAbstractButton *btn = qobject_cast<QAbstractButton*>(sender());
    if (measuring){
        measuring = false;
        btn->setText(tr("开始测量"));
    } else {
        measuring = true;
        btn->setText(tr("停止测量"));
    }
}

void SpectrumModel::on_pushButton_save_clicked()
{
    // 保存数据
}

void SpectrumModel::on_pushButton_gauss_clicked()
{
    // 高斯拟合
}

void SpectrumModel::initCustomPlot()
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

    ui->widget_plot->layout()->addWidget(customPlotWidget_Det1);
    ui->widget_plot->layout()->addWidget(customPlotWidget_Det2);

    connect(customPlotWidget_Det1, SIGNAL(sigMouseDoubleClickEvent()), this, SLOT(slotPlowWidgetDoubleClickEvent()));
    connect(customPlotWidget_Det2, SIGNAL(sigMouseDoubleClickEvent()), this, SLOT(slotPlowWidgetDoubleClickEvent()));
}

void SpectrumModel::slotPlowWidgetDoubleClickEvent()
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
