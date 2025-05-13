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
        ui->tableWidget_calibration->item(i, 0)->setText(QString::number(calibrationData[i][0], 'E', 3));
        for(int j=1; j<6; j++){
            ui->tableWidget_calibration->item(i, j)->setText(QString::number(calibrationData[i][j]));
        }
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

void YieldCalibration::load()
{
    // 加载参数
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/user.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();
        
        QJsonObject jsonCalibration, jsonYield;
        if (jsonObj.contains("YieldCalibration")){
            jsonCalibration = jsonObj["YieldCalibration"].toObject();
            for(int i=0; i<3; i++)
            {
                QString key = QString("Range%1").arg(i);
                QJsonArray  rangeArray;
                if (jsonCalibration.contains(key)){
                    rangeArray = jsonCalibration[key].toArray();
                    QJsonObject rangeData = rangeArray[0].toObject();

                    double yield = rangeData["Yield"].toDouble();
                    double active0 = rangeData["active0"].toDouble();
                    double ratioCu62 = rangeData["branchingRatio_Cu62"].toDouble();
                    double ratioCu64 = rangeData["branchingRatio_Cu64"].toDouble();
                    double backRatesDet1 = rangeData["backgroundRatesDet1"].toDouble();
                    double backRatesDet2 = rangeData["backgroundRatesDet2"].toDouble();
                    
                    calibrationData[i][0] = yield;
                    calibrationData[i][1] = active0;
                    calibrationData[i][2] = ratioCu62;
                    calibrationData[i][3] = ratioCu64;
                    calibrationData[i][4] = backRatesDet1;
                    calibrationData[i][5] = backRatesDet2;
                }
            }
        }
    }
}

bool YieldCalibration::save()
{
    // 保存参数
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);

    QFile file(QApplication::applicationDirPath() + "/config/user.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        QJsonObject jsonRange;
        for(int i=0; i<3; i++)
        {
            QJsonArray items;
            QJsonObject item;
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

            item["Yield"] = yield;
            item["active0"] = active;
            item["branchingRatio_Cu62"] = ratioCu62;
            item["branchingRatio_Cu64"] = ratioCu64;
            item["backgroundRatesDet1"] = backRatesDet1;
            item["backgroundRatesDet2"] = backRatesDet2;

            // 同步成员变量的数值，方便下次检查table是否变化
            calibrationData[i][0] = yield;
            calibrationData[i][1] = active;
            calibrationData[i][2] = ratioCu62;
            calibrationData[i][3] = ratioCu64;
            calibrationData[i][4] = backRatesDet1;
            calibrationData[i][5] = backRatesDet2;
            
            items.append(item);
            jsonRange[key] = items;
        }
        jsonObj["YieldCalibration"] = jsonRange;

        file.open(QIODevice::WriteOnly | QIODevice::Text);
        jsonDoc.setObject(jsonObj);
        file.write(jsonDoc.toJson());
        file.close();
    }
    else
    {
        QJsonObject jsonObj;
        QJsonObject jsonRange;
        for(int i=0; i<3; i++)
        {
            QJsonArray items;
            QJsonObject item;
            QString key = QString("Range%1").arg(i);
            item["Yield"] = calibrationData[i][0];
            item["active0"] = calibrationData[i][1];
            item["branchingRatio_Cu62"] = calibrationData[i][2];
            item["branchingRatio_Cu64"] = calibrationData[i][3];
            item["backgroundRatesDet1"] = calibrationData[i][4];
            item["backgroundRatesDet2"] = calibrationData[i][5];

            items.append(item);
            jsonRange[key] = items;
        }
        jsonObj["YieldCalibration"] = jsonRange;

        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QJsonDocument jsonDoc;
        jsonDoc.setObject(jsonObj);
        file.write(jsonDoc.toJson());
        file.close();
    }
    QMessageBox::information(nullptr, tr("提示"), tr("产额标定数据保存成功！"));
    return true;
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
