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
    //ui->label_tag_->installEventFilter(this);
    //ui->label_tag_2_->installEventFilter(this);
    //ui->label_tag_3_->installEventFilter(this);

    ui->label_tag->setBuddy(ui->widget_tag);
    ui->label_tag_->setBuddy(ui->widget_tag);
    ui->label_tag_2->setBuddy(ui->widget_tag_2);
    ui->label_tag_2_->setBuddy(ui->widget_tag_2);
    ui->label_tag_3->setBuddy(ui->widget_tag_3);
    ui->label_tag_3_->setBuddy(ui->widget_tag_3);

    //ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
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
    PlotWidget *customPlotWidget = new PlotWidget(this);
    customPlotWidget->initCustomPlot();
    ui->widget_plot->layout()->addWidget(customPlotWidget);
}
