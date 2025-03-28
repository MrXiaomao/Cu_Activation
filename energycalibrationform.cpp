#include "energycalibrationform.h"
#include "ui_energycalibrationform.h"
#include "qcustomplot.h"
#include "function.h"

EnergyCalibrationForm::EnergyCalibrationForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EnergyCalibrationForm)
{
    ui->setupUi(this);

    initCustomPlot();

    ui->radioButton_2->setVisible(false);
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

    // 设置选择容忍度，即鼠标点击点到数据点的距离
    //customPlot->setSelectionTolerance(5);
    // 设置全局抗锯齿
    customPlot->setAntialiasedElements(QCP::aeAll);
    //customPlot->setNotAntialiasedElements(QCP::aeAll);
    // 图例名称隐藏
    customPlot->legend->setVisible(false);
    // 图例名称显示位置
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignRight);
    // 设置边界
    customPlot->setContentsMargins(0, 0, 0, 0);
    // 设置标签倾斜角度，避免显示不下
    customPlot->xAxis->setTickLabelRotation(-45);
    // 背景色
    customPlot->setBackground(QBrush(Qt::white));
    // 图像画布边界
    customPlot->axisRect()->setMinimumMargins(QMargins(0, 0, 0, 0));
    // 坐标背景色
    customPlot->axisRect()->setBackground(Qt::white);
    // 允许拖拽，缩放
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    // 允许轴自适应大小
    //customPlot->graph(0)->rescaleValueAxis(true);
//    customPlot->xAxis->rescale(true);
//    customPlot->yAxis->rescale(true);

    // 设置刻度范围
//    QSharedPointer<QCPAxisTickerFixed> axisTickerFixed(new QCPAxisTickerFixed);
//    axisTickerFixed->setTickStep(256);
//    axisTickerFixed->setScaleStrategy(QCPAxisTickerFixed::ssNone);
//    customPlot->xAxis->setTicker(axisTickerFixed);
//    customPlot->xAxis->setRange(0, 2048);
//    customPlot->yAxis->setRange(0, 100);
//    customPlot->yAxis->ticker()->setTickCount(5);
//    customPlot->xAxis->ticker()->setTickCount(8);
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
    customPlot->xAxis->setSubTicks(true);
    customPlot->xAxis2->setSubTicks(false);
    customPlot->yAxis->setSubTicks(true);
    customPlot->yAxis2->setSubTicks(false);
    //设置轴标签名称
    customPlot->xAxis->setLabel(QObject::tr("X"));
    customPlot->yAxis->setLabel(QObject::tr("Y"));
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
    fixedTextTtem = new QCPItemText(customPlot);
    fixedTextTtem->setColor(Qt::black);
    fixedTextTtem->position->setType(QCPItemPosition::ptAbsolute);
    fixedTextTtem->setPositionAlignment(Qt::AlignTop | Qt::AlignLeft);
    fixedTextTtem->setTextAlignment(Qt::AlignLeft);
    fixedTextTtem->setFont(QFont(font().family(), 12));
    fixedTextTtem->setPadding(QMargins(8, 0, 0, 0));
    fixedTextTtem->position->setCoords(20.0, 20.0);
    fixedTextTtem->setText("");

    // 添加散点图
    QCPGraph *curGraphDot = customPlot->addGraph(customPlot->xAxis, customPlot->yAxis);
    curGraphDot->setPen(QPen(Qt::black));
    curGraphDot->setLineStyle(QCPGraph::lsNone);// 取消线性图，改为散点图
    curGraphDot->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));

    QCPGraph *curGraphCurve = customPlot->addGraph(customPlot->xAxis, customPlot->yAxis);
    curGraphCurve->setPen(QPen(Qt::blue, 2, Qt::PenStyle::SolidLine));
    curGraphCurve->setLineStyle(QCPGraph::lsLine);

    connect(customPlot, &QCustomPlot::afterLayout, this, [=](){
        fixedTextTtem->position->setCoords(customPlot->axisRect()->topLeft().x(), customPlot->axisRect()->topLeft().y());
    });
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
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString("%1").arg(ui->doubleSpinBox_x->value())));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString("%1").arg(ui->doubleSpinBox_y->value())));
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

void EnergyCalibrationForm::on_pushButton_2_clicked()
{
    minx = miny = INT_MAX;
    maxx = maxy = -INT_MAX;
    points.clear();
    datafit = false;

    for (int i= 0; i<ui->tableWidget->rowCount(); ++i){
        double x, y;
        x = ui->tableWidget->item(i, 0)->text().toDouble();
        y = ui->tableWidget->item(i, 1)->text().toDouble();
        points.push_back(QPointF(x, y));
    }

    if(ui->radioButton->isChecked())
    {
        //第一个拟合函数
        calculate(1);
    }
    else if(ui->radioButton_2->isChecked())
    {
        //第二个拟合函数
        calculate(2);
    }
}

#define SPAN 18
#define EPS 0.001
extern GETVALUE BASEFUNC[2][3];
void EnergyCalibrationForm::calculate(int no)
{
    datafit = false;
    func = no;

    Matrix A;

    if(no == 1)
    {
        A = Matrix(points.size(), 2);
    }
    else if(no == 2)
    {
        A = Matrix(points.size(), 3);
    }
    for(int j = 0; j < points.size(); j++)
    {
        if(no == 1)
        {
            A.at(j, 0) = BASEFUNC[no-1][0](points[j].x());
            A.at(j, 1) = BASEFUNC[no-1][1](points[j].x());
        }
        else if(no == 2)
        {
            A.at(j, 0) = BASEFUNC[no-1][0](points[j].x());
            A.at(j, 1) = BASEFUNC[no-1][1](points[j].x());
            A.at(j, 2) = BASEFUNC[no-1][2](points[j].x());
        }
    }

//    for (int i = 0; i < A.mm(); i++)
//    {
//        for(int j = 0; j < A.nn(); j++)
//        {
//            qDebug() << A.at(i, j);
//        }
//        qDebug() << "---------------";
//    }
//    return ;
    //---------
    Matrix detA = A.reverse();

    Matrix W = detA * A;

    Matrix y(points.size(), 1);
    for(int i = 0; i < points.size(); i++)
    {
        y.at(i, 0) = points[i].y();
    }
    Matrix Y = detA * y;

    Matrix maze = W.append(Y);

    for (int i = 0; i < maze.mm(); i++)
    {
        for(int j = 0; j < maze.nn(); j++)
        {
            qDebug() << maze.at(i, j);
        }
    }
    //        return;
    //开始计算参数
    solveC(maze);

    if(no == 1)
    {
        if (C[1] > 0)
            fixedTextTtem->setText(QString("y = %1 + %2 * x").arg(C[0]).arg(C[1]));
        else
            fixedTextTtem->setText(QString("y = %1 - %2 * x").arg(C[0]).arg(qAbs(C[1])));
    }
    else
    {
        fixedTextTtem->setText(QString("y = %1 / x + %2  + %3 * x").arg(C[0]).arg(C[1]).arg(C[2]));
    }

    maxx = -INT_MAX, minx = INT_MAX;
    maxy = -INT_MAX, miny = INT_MAX;
    for (int i = 0; i < points.size(); i++) {

        if(points[i].x() > maxx)
        {
            maxx = points[i].x();
        }
        if(points[i].x() < minx)
        {
            minx = points[i].x();
        }

        if(points[i].y() > maxy)
        {
            maxy = points[i].y();
        }
        if(points[i].y() < miny)
        {
            miny = points[i].y();
        }
    }

    double xRangLower = 0, xRangUpper = 0;
    double yRangLower = 0, yRangUpper = 0;

        //绘制点
    {
        QVector<double> keys, values;
        for (int i = 0; i < points.size(); i++)
        {
            qreal x = points[i].x(), y = points[i].y();
            keys << x;
            values << y;
            yRangUpper = qMax(yRangUpper, y);
        }

        xRangUpper = points.size() + 0.5;
        customPlot->graph(0)->setData(keys, values);
    }

    if(datafit)
    {
        QVector<double> keys, values;
        qreal x = minx;
        qreal y;
        if(ui->radioButton->isChecked())
        {
            y = C[0] + C[1] * x;
        }
        else//(ui->radioButton_2->isChecked())
        {
            y = C[0] / x + C[1] + C[2] * x;
        }

        for (qreal i = x+0.01; i <= maxx; i += 0.01)
        {
            qreal nx = i;
            qreal ny;
            if(ui->radioButton->isChecked())
            {
                ny = C[0] + C[1] * x;
            }
            else //(ui->radioButton_2->isChecked())
            {
                ny = C[0] / x + C[1] + C[2] * x;
            }

            x = nx;
            y = ny;

            keys << nx;
            values << ny;
        }

        customPlot->graph(1)->setData(keys, values);
    }

    customPlot->xAxis->setRange(xRangLower, xRangUpper);
    customPlot->yAxis->setRange(yRangLower, yRangUpper);
//    customPlot->xAxis->rescale(true);
//    customPlot->yAxis->rescale(true);
    customPlot->replot();
}


void EnergyCalibrationForm::solveC(Matrix maze)
{
    bool flag = 1;
    for(int i = 0; i < maze.mm() - 1; i++)
    {
        //找出主元最大的
        int k = i;
        for(int j = i+1; j < maze.mm(); j++)
        {
            if(maze.at(j ,i ) > maze.at(i, i)) k = j;
        }

        if(maze.at(k, i) < EPS - 1e-9)
        {
            flag = 0;
            break;
        }

        //跟第i行交换
        if(k != i)
        {
            for(int e = i; e < maze.nn(); e++)
            {
                qreal x = maze.at(i, e);
                maze.at(i, e) = maze.at(k, e);
                maze.at(k, e) = x;
            }
        }

        //计算 化简
        for(int j = i+1; j < maze.mm(); j++)
        {
            double ratio = 	maze.at(j, i) / maze.at(i, i);

            for(int z = i; z < maze.nn(); z++)
            {
                maze.at(j, z) += (-1) * ratio * maze.at(i, z);
            }
        }
    }

    if(!flag)
    {
        QMessageBox::warning(this, tr("警告"), tr("主元素太小，无法计算！"));
        return;
    }
    else
    {
        int n = maze.mm() - 1;
        //回代
        C[n] = maze.at(n, n+1 ) / maze.at(n, n);
        for(int i = n-1; i>=0; i--)
        {
            qreal sum = 0.0;
            for(int j = i+1; j <=n; j++)
            {
                sum += C[j] * maze.at(i, j);
            }
            C[i] = (maze.at(i, n+1) - sum) / maze.at(i, i);
        }

        for(int i = 0; i <=n; i++)
        {
            qDebug() << i << " " << C[i];
        }

        datafit = 1;
    }
}

void EnergyCalibrationForm::on_pushButton_clear_clicked()
{
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
}
