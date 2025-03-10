#include "energycalibrationform.h"
#include "ui_energycalibrationform.h"
#include "qcustomplot.h"

EnergyCalibrationForm::EnergyCalibrationForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EnergyCalibrationForm)
{
    ui->setupUi(this);

    initCustomPlot();
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

EnergyCalibrationForm::~EnergyCalibrationForm()
{
    delete ui;
}

void EnergyCalibrationForm::initCustomPlot()
{
    // 创建绘图区域
    customPlot = new QCustomPlot(ui->widget_plot);
    customPlot->setObjectName("customPlot");
    customPlot->setLocale(QLocale(QLocale::Chinese, QLocale::China));
    ui->widget_plot->layout()->addWidget(customPlot);

    // 图例名称隐藏
    customPlot->legend->setVisible(false);
    // 图例名称显示位置
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignRight);

    // 设置刻度范围
    QSharedPointer<QCPAxisTickerFixed> axisTickerFixed(new QCPAxisTickerFixed);
    axisTickerFixed->setTickStep(256);
    axisTickerFixed->setScaleStrategy(QCPAxisTickerFixed::ssNone);
    customPlot->xAxis->setTicker(axisTickerFixed);
    customPlot->xAxis->setRange(0, 2048);
    customPlot->yAxis->setRange(0, 100);
    customPlot->yAxis->ticker()->setTickCount(5);
    customPlot->xAxis->ticker()->setTickCount(8);
    // 设置刻度可见
    customPlot->xAxis->setTicks(true);
    customPlot->xAxis2->setTicks(false);
    customPlot->yAxis->setTicks(true);
    customPlot->yAxis2->setTicks(false);
    // 设置轴线可见
    customPlot->xAxis->setVisible(true);
    customPlot->xAxis2->setVisible(true);
    customPlot->yAxis->setVisible(true);
    customPlot->yAxis2->setVisible(true);
    //customPlot->axisRect()->setupFullAxesBox();//四边安装轴并显示
    // 设置刻度标签可见
    customPlot->xAxis->setTickLabels(true);
    customPlot->xAxis2->setTickLabels(false);
    customPlot->yAxis->setTickLabels(true);
    customPlot->yAxis2->setTickLabels(false);
    // 设置子刻度可见
    customPlot->xAxis->setSubTicks(false);
    customPlot->xAxis2->setSubTicks(false);
    customPlot->yAxis->setSubTicks(false);
    customPlot->yAxis2->setSubTicks(false);
    //设置轴标签名称
    customPlot->xAxis->setLabel(QObject::tr("道数"));
    customPlot->yAxis->setLabel(QObject::tr("能量/Kev"));
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

    // 文本元素随窗口变动而变动
    QCPItemText *fixedTextTtem = new QCPItemText(customPlot);
    fixedTextTtem->setColor(Qt::white);
    fixedTextTtem->position->setType(QCPItemPosition::ptAbsolute);
    fixedTextTtem->setPositionAlignment(Qt::AlignTop | Qt::AlignLeft);
    fixedTextTtem->setTextAlignment(Qt::AlignLeft);
    fixedTextTtem->setFont(QFont(font().family(), 12));
    fixedTextTtem->setPadding(QMargins(8, 0, 0, 0));
    fixedTextTtem->position->setCoords(10.0, 10.0);//窗口坐标值
    fixedTextTtem->setText("E=-nan(ind) + -nan(ind)*CH\nR^2=1.000");

    // 添加散点图
    QCPGraph * curGraph = customPlot->addGraph(customPlot->xAxis, customPlot->yAxis);
    Q_UNUSED(curGraph);
    customPlot->graph(0)->setPen(QPen(Qt::blue));
    //customPlot->graph(0)->setBrush(QBrush(Qt::blue));
    //customPlot->graph(0)->setLineStyle(QCPGraph::lsLine);
    customPlot->graph(0)->setLineStyle(QCPGraph::lsNone);// 取消线性图，改为散点图
    customPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 3));

    // 图形刷新
    customPlot->replot();

//    QTimer *timerUpdate = new QTimer(this);
//    static int timerRef = 0;
//    connect(timerUpdate, &QTimer::timeout, this, [=](){
//        // 更新数据
//        customPlot->graph(0)->addData(timerRef++, qrand() % 5 + 80);

//        // 保持图形范围合理
//        if (timerRef >= 2048)
//            customPlot->xAxis->setRange(timerRef - 2048, 2048, Qt::AlignRight);

//        // 重新绘制
//        customPlot->replot();
//    });
//    timerUpdate->start(50);
}

void EnergyCalibrationForm::on_pushButton_add_clicked()
{
    // 添加
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString("%1").arg(ui->lineEdit->text())));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString("%1").arg(ui->lineEdit_2->text())));
}

void EnergyCalibrationForm::on_pushButton_del_clicked()
{
    // 删除
    QList<QTableWidgetItem*> selItems = ui->tableWidget->selectedItems();
    if (selItems.count() <= 0)
        return;

    QTableWidgetItem* selItem = selItems[0];
    int row = selItem->row();
    ui->tableWidget->removeRow(row);
}
