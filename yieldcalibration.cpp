#include "yieldcalibration.h"
#include "ui_yieldcalibration.h"
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

YieldCalibration::YieldCalibration(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::YieldCalibration)
{
    ui->setupUi(this);
    //小量程
    calibrationData[0][0] = 1.0e3; 
    calibrationData[0][1] = 1.0e3; 
    calibrationData[0][2] = 60.0; 
    calibrationData[0][3] = 40.0; 
    calibrationData[0][4] = 0.1;
    calibrationData[0][5] = 0.1;
    //中量程
    calibrationData[1][0] = 1.0e7; 
    calibrationData[1][1] = 1.0e3; 
    calibrationData[1][2] = 60.0; 
    calibrationData[1][3] = 40.0; 
    calibrationData[1][4] = 0.1;
    calibrationData[1][5] = 0.1;
    //大量程
    calibrationData[2][0] = 1.0e11; 
    calibrationData[2][1] = 1.0e3; 
    calibrationData[2][2] = 60.0; 
    calibrationData[2][3] = 40.0; 
    calibrationData[2][4] = 0.1;
    calibrationData[2][5] = 0.1;

    this->load();

    for(int i=0; i<3; i++){
        //注意合理控制精度，避免丢失精度，第三个参数是小数位数
        ui->tableWidget_calibration->item(i, 0)->setText(QString::number(calibrationData[i][0], 'E', 6)); //该数值较大，采用科学计数法更佳
        ui->tableWidget_calibration->item(i, 1)->setText(QString::number(calibrationData[i][1], 'g', 9)); //该数值一般不超过10^6，不用科学计数

        //这两个精度要求非常高,第三个参数是设置有效数字位数，而不是小数位数
        ui->tableWidget_calibration->item(i, 2)->setText(QString::number(calibrationData[i][2], 'g', 9));
        ui->tableWidget_calibration->item(i, 3)->setText(QString::number(calibrationData[i][3], 'g', 9));

        ui->tableWidget_calibration->item(i, 4)->setText(QString::number(calibrationData[i][4], 'g', 6));
        ui->tableWidget_calibration->item(i, 5)->setText(QString::number(calibrationData[i][5], 'g', 6));

        // for(int j=1; j<6; j++){
        //     ui->tableWidget_calibration->item(i, j)->setText(QString::number(calibrationData[i][j]));
        // }
    }

    // 设置表头换行
    ui->tableWidget_calibration->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget_calibration->horizontalHeader()->setStyleSheet("QHeaderView::section { white-space: pre-wrap; }");
    ui->tableWidget_calibration->setHorizontalHeaderItem(0, new QTableWidgetItem("产额"));
    ui->tableWidget_calibration->setHorizontalHeaderItem(1, new QTableWidgetItem("相对初始活度"));
    ui->tableWidget_calibration->setHorizontalHeaderItem(2, new QTableWidgetItem("Cu62初始\n活度份额(%)"));
    ui->tableWidget_calibration->setHorizontalHeaderItem(3, new QTableWidgetItem("Cu64初始\n活度份额(%)"));
    ui->tableWidget_calibration->setHorizontalHeaderItem(4, new QTableWidgetItem("探测器1\n本底计数率(cps)"));
    ui->tableWidget_calibration->setHorizontalHeaderItem(5, new QTableWidgetItem("探测器2\n本底计数率(cps)"));

    //最后一列自动拉伸
    ui->tableWidget_calibration->horizontalHeader()->setStretchLastSection(true);

    QFont font("Microsoft YaHei", 10); // 设置字体为微软雅黑，大小为10
    ui->label_attention->setFont(font);
    // ui->label_attention->setText("<p style='line-height:20px'>"
    //                       "<font style ='font-size:19px; color:#ffff00;font-weight:bold'> 客户端界面 </font>"
    //                       "<p style='line-height:7px'>"
    //                       "<font style = 'font-size:12px; color:#ffffff; '> 工具可模拟TCP/UDP通信 </font>"
    //                       );

    // 连接单元格变化信号
    // 连接cellChanged信号
    connect(ui->tableWidget_calibration, &QTableWidget::cellChanged, [this](int row, int col) {
        QTableWidgetItem *item = ui->tableWidget_calibration->item(row, col);
        if (!item) return;
        
        QString text = item->text();
        bool ok;
        double value = text.toDouble(&ok);
        
        if (!ok) {
            // 标记无效输入
            item->setBackground(Qt::red);
            item->setToolTip(tr("无效的数值输入。\n请输入浮点数类型，如：1000, 1000.0, 1E3, 1E+3, 1.0E3, 1.0E+03"));
            
            // 可以恢复之前的值或保持标记
            // item->setText(m_lastValidValue);
        } else {
            // 有效输入
            item->setBackground(Qt::white);
            item->setToolTip("");
            // m_lastValidValue = text; // 保存有效值
        }
    });
}

YieldCalibration::~YieldCalibration()
{
    delete ui;
}

// 检查整个表格是否有修改
bool YieldCalibration::isTableModified(QTableWidget *table)
{
    bool changed = false;
    for (int row = 0; row < table->rowCount(); ++row) {
        for(int colum = 0; colum<6; colum++)
        {
            QTableWidgetItem *item = table->item(row, colum);
            if (item->text().toDouble() != calibrationData[row][colum]) {
                changed = true;
            }
        }
    }
    return changed;
}

void YieldCalibration::on_pushButton_OK_clicked()
{
    if (QMessageBox::question(this, tr("提示"), tr("您确定要更新标定数据吗？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
        return ;

    if(!this->save()) return;
    this->close();
}


void YieldCalibration::on_pushButton_cancel_clicked()
{
    this->close();
}

#include <QThread>
#include "globalsettings.h"
void YieldCalibration::load()
{
    // 加载参数
    JsonSettings* userSettings = GlobalSettings::instance()->mUserSettings;
    if (userSettings->isOpen()){
        userSettings->prepare();
        userSettings->beginGroup("YieldCalibration");
        for(int i=0; i<3; i++)
        {
            QString key = QString("Range%1").arg(i);

            double yield = userSettings->arrayValue(key, 0, "Yield", i==0 ? 10000 : (i==1 ? 10000000 : 100000000000)).toDouble();
            double active0 = userSettings->arrayValue(key, 0, "active0", i==0 ? 5000 : (i==1 ? 6000 : 3333)).toDouble();
            double ratioCu62 = userSettings->arrayValue(key, 0, "branchingRatio_Cu62", i==0 ? 0.12 : (i==1 ? 0.12 : 0.1)).toDouble();
            double ratioCu64 = userSettings->arrayValue(key, 0, "branchingRatio_Cu64", i==0 ? 0.1 : (i==1 ? 0.1 : 0.2)).toDouble();
            double backRatesDet1 = userSettings->arrayValue(key, 0, "backgroundRatesDet1", 0.6).toDouble();
            double backRatesDet2 = userSettings->arrayValue(key, 0, "backgroundRatesDet2", 0.4).toDouble();

            calibrationData[i][0] = yield;
            calibrationData[i][1] = active0;
            calibrationData[i][2] = ratioCu62;
            calibrationData[i][3] = ratioCu64;
            calibrationData[i][4] = backRatesDet1;
            calibrationData[i][5] = backRatesDet2;
        }
        userSettings->endGroup();
        userSettings->finish();
    }
}

bool YieldCalibration::save()
{
    // 保存参数
    JsonSettings* userSettings = GlobalSettings::instance()->mUserSettings;
    if (userSettings->isOpen()){
        userSettings->prepare();
        userSettings->beginGroup("YieldCalibration");
        for(int i=0; i<3; i++)
        {
            QString key = QString("Range%1").arg(i);

            bool ok;
            double yield = ui->tableWidget_calibration->item(i, 0)->text().toDouble(&ok);
            if(!ok) {
                QMessageBox::information(nullptr, tr("提示"), QString("保存失败：存在非法输入！请检查后保存。行：%1,列:%2").arg(i+1).arg(1));
                return false;
            }

            double active = ui->tableWidget_calibration->item(i, 1)->text().toDouble(&ok);
            if(!ok) {
                QMessageBox::information(nullptr, tr("提示"), QString("保存失败：存在非法输入！请检查后保存。行：%1,列:%2").arg(i+1).arg(2));
                return false;
            }

            double ratioCu62 = ui->tableWidget_calibration->item(i, 2)->text().toDouble(&ok);
            if(!ok) {
                QMessageBox::information(nullptr, tr("提示"), QString("保存失败：存在非法输入！请检查后保存。行：%1,列:%2").arg(i+1).arg(3));
                return false;
            }

            double ratioCu64 = ui->tableWidget_calibration->item(i, 3)->text().toDouble(&ok);
            if(!ok) {
                QMessageBox::information(nullptr, tr("提示"), QString("保存失败：存在非法输入！请检查后保存。行：%1,列:%2").arg(i+1).arg(4));
                return false;
            }

            double backRatesDet1 = ui->tableWidget_calibration->item(i, 4)->text().toDouble(&ok);
            if(!ok) {
                QMessageBox::information(nullptr, tr("提示"), QString("保存失败：存在非法输入！请检查后保存。行：%1,列:%2").arg(i+1).arg(5));
                return false;
            }

            double backRatesDet2 = ui->tableWidget_calibration->item(i, 5)->text().toDouble(&ok);
            if(!ok) {
                QMessageBox::information(nullptr, tr("提示"), QString("保存失败：存在非法输入！请检查后保存。行：%1,列:%2").arg(i+1).arg(6));
                return false;
            }

            userSettings->setArrayValue(key, 0, "Yield", yield);
            userSettings->setArrayValue(key, 0, "active0", active);
            userSettings->setArrayValue(key, 0, "branchingRatio_Cu62", ratioCu62);
            userSettings->setArrayValue(key, 0, "branchingRatio_Cu64", ratioCu64);
            userSettings->setArrayValue(key, 0, "backgroundRatesDet1", backRatesDet1);
            userSettings->setArrayValue(key, 0, "backgroundRatesDet2", backRatesDet2);

            // 同步成员变量的数值，方便下次检查table是否变化
            calibrationData[i][0] = yield;
            calibrationData[i][1] = active;
            calibrationData[i][2] = ratioCu62;
            calibrationData[i][3] = ratioCu64;
            calibrationData[i][4] = backRatesDet1;
            calibrationData[i][5] = backRatesDet2;
        }

        userSettings->endGroup();
        bool result = userSettings->flush();
        userSettings->finish();
        if (result){
            QMessageBox::information(nullptr, tr("提示"), tr("产额标定数据保存成功！"));
            return true;
        }
    }

    QMessageBox::information(nullptr, tr("提示"), tr("产额标定数据保存失败！"));
    return false;
}

#include <QCloseEvent>
void YieldCalibration::closeEvent(QCloseEvent *event)
{
    // 用户不确认则不保存，或者检查用户更改后提醒用户是否保存
    bool changed = isTableModified(ui->tableWidget_calibration);
    if(changed){
        if (QMessageBox::question(this, tr("提示"), tr("数据发生更改，是否保存数据？"),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
        {
            this->save();
            event->accept();
            return ;
        }
    }
}
