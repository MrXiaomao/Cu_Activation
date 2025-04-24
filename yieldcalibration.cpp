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
    mapCalibration.insert(0, caliPair(1.0E4, 1111.0));//量程1
    mapCalibration.insert(1, caliPair(1.0E7, 2222.0));//量程2
    mapCalibration.insert(2, caliPair(1.0E11, 3333.0));//量程3
    mapCalibration.insert(3, caliPair(1.0E15, 4444.0));//量程4
    this->load();

    ui->tableWidget_calibration->item(0, 0)->setText(QString::number(mapCalibration[0].yield, 'E', 3));
    ui->tableWidget_calibration->item(1, 0)->setText(QString::number(mapCalibration[1].yield, 'E', 3));
    ui->tableWidget_calibration->item(2, 0)->setText(QString::number(mapCalibration[2].yield, 'E', 3));
    ui->tableWidget_calibration->item(3, 0)->setText(QString::number(mapCalibration[3].yield, 'E', 3));

    ui->tableWidget_calibration->item(0, 1)->setText(QString::number(mapCalibration[0].active_t0, 'E', 5));
    ui->tableWidget_calibration->item(1, 1)->setText(QString::number(mapCalibration[1].active_t0, 'E', 5));
    ui->tableWidget_calibration->item(2, 1)->setText(QString::number(mapCalibration[2].active_t0, 'E', 5));
    ui->tableWidget_calibration->item(3, 1)->setText(QString::number(mapCalibration[3].active_t0, 'E', 5));

    // 连接单元格变化信号
    // connect(tableWidget_calibration, &QTableWidget::cellChanged, this, &YieldCalibration::onCellChanged);

    // 设置水平表头文本换行
    // ui->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    // ui->tableWidget->setWordWrap(true);
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
        QTableWidgetItem *item = table->item(row, 0);
        if (item->text().toDouble() != mapCalibration[row].yield) {
            changed = true;
        }

        QTableWidgetItem *item2 = table->item(row, 1);
        if (item->text().toDouble() != mapCalibration[row].active_t0) {
            changed = true;
        }
    }
    return changed;
}

void YieldCalibration::on_pushButton_OK_clicked()
{
    if (QMessageBox::question(this, tr("提示"), tr("您确定用更新标定数据吗？\r\n\r\n请仔细检查后保存"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
        return ;

    this->save();
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
            for(int i=0; i<4; i++)
            {
                QString key = QString("Range%1").arg(i);
                QJsonArray  rangeArray;
                if (jsonCalibration.contains(key)){
                    rangeArray = jsonCalibration[key].toArray();
                    QJsonObject rangeData = rangeArray[0].toObject();

                    double yield = rangeData["Yield"].toDouble();
                    double active0 = rangeData["active0"].toDouble();
                    mapCalibration[i] = caliPair(yield, active0);
                }
            }
        }
    }
}

void YieldCalibration::save()
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
        for(int i=0; i<4; i++)
        {
            QJsonArray items;
            QJsonObject item;
            QString key = QString("Range%1").arg(i);
            double yield = ui->tableWidget_calibration->item(i, 0)->text().toDouble();
            double active = ui->tableWidget_calibration->item(i, 1)->text().toDouble();
            item["Yield"] = yield;
            item["active0"] = active;
            // 同步成员变量的数值，方便下次检查table是否变化
            mapCalibration[i].yield = yield;
            mapCalibration[i].active_t0 = active;

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
        for(int i=0; i<4; i++)
        {
            QJsonArray items;
            QJsonObject item;
            QString key = QString("Range%1").arg(i);
            item["Yield"] = mapCalibration[i].yield;
            item["active0"] = mapCalibration[i].active_t0;
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
}

void YieldCalibration::closeEvent(QCloseEvent */*event*/)
{
    // 用户不确认则不保存，或者检查用户更改后提醒用户是否保存
    bool changed = isTableModified(ui->tableWidget_calibration);
    if(changed){
        if (QMessageBox::question(this, tr("提示"), tr("数据发生更改，是否保存数据？"),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
        {
            this->save();
            return ;
        }
    }
}
