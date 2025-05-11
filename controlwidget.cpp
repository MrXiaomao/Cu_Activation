#include "controlwidget.h"
#include "ui_controlwidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QButtonGroup>
#include <QMessageBox>

ControlWidget::ControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlWidget)
{
    ui->setupUi(this);
    controlHelper = ControlHelper::instance();

    QButtonGroup* grp = new QButtonGroup();
    grp->setExclusive(true);
    grp->addButton(ui->radioButton_01, 0);
    grp->addButton(ui->radioButton_02, 1);
    connect(grp, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [=](int index){
        this->save();
        mAxis_no = index + 1;
        if (0 == index){
            ui->groupBox->setTitle(tr("轴1"));
        }
        else{
            ui->groupBox->setTitle(tr("轴2"));
        }

        this->load();        
    });    
    enableUiStatus();

    // 打印SDK版本
    ui->tableWidget_position->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

//    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();		//获取设备的串口列表
//    for (int i = 0; i < ports.size();i++)
//    {
//        for (int j = i + 1; j < ports.size(); j++)
//        {
//            QString name = ports.at(i).portName();	//i的串口数字
//            int portNumI = name.mid(3).toInt();

//            name = ports.at(j).portName();			//j的串口数字
//            int portNumJ = name.mid(3).toInt();

//            if (portNumI > portNumJ)				//ij交换
//            {
//                ports.swap(i, j);
//            }
//        }
//    }

//    for (auto port : ports)
//    {
//        ui->comboBox_com->addItem(port.portName());
//        ui->comboBox_com_2->addItem(port.portName());
//    }

    //量程
    //负限位
    //正限位
    for (int i=0; i<=5; ++i){
        QTableWidget* tableWidgets[] = {ui->tableWidget_position};
        for (auto tableWidget : tableWidgets){
            tableWidget->item(i, 0)->setFlags(tableWidget->item(i, 0)->flags() & ~Qt::ItemFlag::ItemIsEditable);
            tableWidget->setItem(i, 1, new QTableWidgetItem());
            tableWidget->item(i, 1)->setTextAlignment(Qt::AlignCenter);
            //tableWidget->item(row, 0)->setFlags(ui->tableWidget->item(row, 0)->flags() & (~Qt::ItemIsEditable));

            QWidget *w = new QWidget(tableWidget);
            w->setLayout(new QHBoxLayout());
            QPushButton *btn1 = new QPushButton(tr("设定"));
            QPushButton *btn2 = new QPushButton(tr("Go->"));
            tableWidget->setCellWidget(i, 2, w);
            w->layout()->setContentsMargins(0,0,0,0);
            w->layout()->addWidget(btn1);
            w->layout()->addWidget(btn2);
            connect(btn1, &QPushButton::clicked, this, [&,tableWidget,i](){
                if (mAxis_no == 0x01)
                    tableWidget->item(i, 1)->setText(ui->lineEdit_position->text());
                else
                    tableWidget->item(i, 1)->setText(ui->lineEdit_position_2->text());
            });
            connect(btn2, &QPushButton::clicked, this, [&,tableWidget,i](){
                float v = tableWidget->item(i, 1)->text().toFloat();
                if (i<=3) //量程
                    controlHelper->single_moveabs(mAxis_no, v * 1000);
                else if (4==i)//设定负限位
                    controlHelper->set_sw_p1(mAxis_no, v * 1000);
                else if (5==i)//设定正限位
                    controlHelper->set_sw_p2(mAxis_no, v * 1000);
            });
            btn1->setProperty("row", i);
            btn2->setProperty("row", i);
        }
    }

    ui->pushButton_forward->installEventFilter(this);
    ui->pushButton_backward->installEventFilter(this);

    connect(controlHelper, &ControlHelper::sigReportAbs, this, [=](float f1, float f2){
        ui->lineEdit_position->setText(QString::number(f1 / 1000, 'f', 4));
        ui->lineEdit_position_2->setText(QString::number(f2 / 1000, 'f', 4));

        if (this->property("enableAuto").toBool()){
            float realPos1 = f1 / 1000;
            float realPos2 = f2 / 1000;

            // 0.003误差3mm
            if (MAX_MISTAKE_VALUE > qAbs(realPos1 - ui->doubleSpinBox_lower->value()) || realPos1 <= ui->doubleSpinBox_lower->value()){
                controlHelper->single_stop(0x01);
                QTimer::singleShot(ui->spinBox_delay->value(), this, [=](){
                    //controlHelper->single_moveabs(0x01, ui->doubleSpinBox_upper->value() * 1000);
                    controlHelper->single_jogright(0x01);

                });
            } else if (MAX_MISTAKE_VALUE > qAbs(realPos1 - ui->doubleSpinBox_upper->value()) || realPos1 >= ui->doubleSpinBox_upper->value()){
                controlHelper->single_stop(0x01);
                QTimer::singleShot(ui->spinBox_delay->value(), this, [=](){
                    //controlHelper->single_moveabs(0x01, ui->doubleSpinBox_lower->value() * 1000);
                    controlHelper->single_jogleft(0x01);
                });
            }

            if (MAX_MISTAKE_VALUE > qAbs(realPos2 - ui->doubleSpinBox_lower->value()) || realPos2 <= ui->doubleSpinBox_lower->value()){
                controlHelper->single_stop(0x02);
                QTimer::singleShot(ui->spinBox_delay->value(), this, [=](){
                    //controlHelper->single_moveabs(0x02, ui->doubleSpinBox_upper->value() * 1000);
                    controlHelper->single_jogright(0x02);
                });
            } else if (MAX_MISTAKE_VALUE > qAbs(realPos2 - ui->doubleSpinBox_upper->value()) || realPos2 >= ui->doubleSpinBox_upper->value()){
                controlHelper->single_stop(0x02);
                QTimer::singleShot(ui->spinBox_delay->value(), this, [=](){
                    //controlHelper->single_moveabs(0x02, ui->doubleSpinBox_lower->value() * 1000);
                    controlHelper->single_jogleft(0x02);
                });
            }
        }
    });
    connect(controlHelper, &ControlHelper::sigReportStatus, this, [=](int axis_no, bool limit_left, bool limit_right, bool isrunning){
        if (mAxis_no == axis_no){
            if (isrunning == FT_TRUE){
                ui->label_running->setStyleSheet("min-width:16px;"
                                                 "min-height:16px;"
                                                 "max-width:16px;"
                                                 "max-height:16px;"
                                                 "border-radius: 8px;"
                                                 "border:1px solid rgb(200, 200, 200);"
                                                 "background-color: rgb(170, 255, 0);");
            } else {
                ui->label_running->setStyleSheet("min-width:16px;"
                                                 "min-height:16px;"
                                                 "max-width:16px;"
                                                 "max-height:16px;"
                                                 "border-radius: 8px;"
                                                 "border:1px solid rgb(200, 200, 200);"
                                                 "background-color: rgb(96, 96, 96);");
            }

            if (limit_right == FT_TRUE){
                ui->pushButton_forward->setEnabled(false);
                ui->label_positive->setStyleSheet("min-width:16px;"
                                                  "min-height:16px;"
                                                  "max-width:16px;"
                                                  "max-height:16px;"
                                                  "border-radius: 8px;"
                                                  "border:1px solid rgb(200, 200, 200);"
                                                  "background-color: rgb(170, 255, 0);");
            } else {
                ui->pushButton_forward->setEnabled(true);
                ui->label_positive->setStyleSheet("min-width:16px;"
                                                  "min-height:16px;"
                                                  "max-width:16px;"
                                                  "max-height:16px;"
                                                  "border-radius: 8px;"
                                                  "border:1px solid rgb(200, 200, 200);"
                                                  "background-color: rgb(96, 96, 96);");
            }

            if (limit_left == FT_TRUE){
                ui->pushButton_backward->setEnabled(true);
                ui->label_negative->setStyleSheet("min-width:16px;"
                                                  "min-height:16px;"
                                                  "max-width:16px;"
                                                  "max-height:16px;"
                                                  "border-radius: 8px;"
                                                  "border:1px solid rgb(200, 200, 200);"
                                                  "background-color: rgb(170, 255, 0);");
            } else {
                ui->pushButton_backward->setEnabled(true);
                ui->label_negative->setStyleSheet("min-width:16px;"
                                                  "min-height:16px;"
                                                  "max-width:16px;"
                                                  "max-height:16px;"
                                                  "border-radius: 8px;"
                                                  "border:1px solid rgb(200, 200, 200);"
                                                  "background-color: rgb(96, 96, 96);");
            }
        }
    });

    this->load();
}

ControlWidget::~ControlWidget()
{
    disconnect(controlHelper, nullptr);

    delete ui;
}

void ControlWidget::on_pushButton_zero_clicked()
{
    //回零
    controlHelper->single_home(mAxis_no);
}

void ControlWidget::on_pushButton_stop_clicked()
{
    //停止
    controlHelper->single_stop(mAxis_no);
}

void ControlWidget::on_pushButton_absolute_position_clicked()
{
    //绝对位置
    controlHelper->single_moveabs(mAxis_no, ui->doubleSpinBox_absolute_position->value() * 1000);
}

void ControlWidget::on_pushButton_relative_position_clicked()
{
    //相对位置
    controlHelper->single_move(mAxis_no, ui->doubleSpinBox_relative_position->value() * 1000);
}

void ControlWidget::on_pushButton_speed_clicked()
{
    //速度    
    controlHelper->set_vel(mAxis_no, ui->doubleSpinBox_speed->value());
}

void ControlWidget::on_pushButton_setup_clicked()
{
    //参数配置
    controlHelper->save_params_permanently(mAxis_no);
}

void ControlWidget::load()
{
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        if (jsonObj.contains("Control")){            
            QJsonObject jsonControl = jsonObj["Control"].toObject();
            QJsonArray jsonDistances = jsonControl["Distances"].toArray();

            if (jsonControl.contains("max_speed")){
                float max_speed = jsonControl["max_speed"].toDouble();
                ui->doubleSpinBox_speed->setMaximum(max_speed);
            }

            if (jsonControl.contains("Distances")){
                QJsonObject jsonDistance1 = jsonControl["Distances"].toObject()["01"].toObject();
                QJsonObject jsonDistance2 = jsonControl["Distances"].toObject()["02"].toObject();
                if (mAxis_no == 0x01){
                    ui->tableWidget_position->item(0, 1)->setText(jsonDistance1["smallRange"].toString());
                    ui->tableWidget_position->item(1, 1)->setText(jsonDistance1["mediumRange"].toString());
                    ui->tableWidget_position->item(2, 1)->setText(jsonDistance1["largeRange"].toString());
                } else {
                    ui->tableWidget_position->item(0, 1)->setText(jsonDistance2["smallRange"].toString());
                    ui->tableWidget_position->item(1, 1)->setText(jsonDistance2["mediumRange"].toString());
                    ui->tableWidget_position->item(2, 1)->setText(jsonDistance2["largeRange"].toString());
                }
            }
        }
    }

    float dValue;
    //设定负限位
    controlHelper->get_sw_p1(mAxis_no, &dValue);
    ui->tableWidget_position->item(4, 1)->setText(QString::number(dValue / 1000, 'f', 4));
    //设定正限位
    controlHelper->get_sw_p2(mAxis_no, &dValue);
    ui->tableWidget_position->item(5, 1)->setText(QString::number(dValue / 1000, 'f', 4));

    //运动速度
    controlHelper->get_vel(mAxis_no, &dValue);
    ui->doubleSpinBox_speed->setValue(dValue);
}

bool ControlWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != this){
        if (event->type() == QEvent::MouseButtonPress){
            if (watched->inherits("QPushButton")){
                QPushButton* btn = qobject_cast<QPushButton*>(watched);
                if (btn->objectName() == "pushButton_forward" || btn->objectName() == "pushButton_forward_2"){
                    //正向
                    controlHelper->single_stop(mAxis_no);
                    controlHelper->single_jogright(mAxis_no);
                } else if (btn->objectName() == "pushButton_backward" || btn->objectName() == "pushButton_backward_2"){
                    //负向
                    controlHelper->single_stop(mAxis_no);
                    controlHelper->single_jogleft(mAxis_no);
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease){
            if (watched->inherits("QPushButton")){
                controlHelper->single_stop(mAxis_no);
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void ControlWidget::save()
{
    // 保存参数
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        QJsonObject jsonControl, jsonDistances, jsonDistance1, jsonDistance2;

        if (jsonObj.contains("Control")){
            jsonControl = jsonObj["Control"].toObject();
        }
        if (jsonControl.contains("Distances")){
            jsonDistances = jsonControl["Distances"].toObject();
            jsonDistance1 = jsonDistances["01"].toObject();
            jsonDistance2 = jsonDistances["02"].toObject();
        }

        if (mAxis_no == 0x01){
            jsonDistance1["smallRange"] = ui->tableWidget_position->item(0, 1)->text();
            jsonDistance1["mediumRange"] = ui->tableWidget_position->item(1, 1)->text();
            jsonDistance1["largeRange"] = ui->tableWidget_position->item(2, 1)->text();
            jsonDistance1["1E10-1E13"] = ui->tableWidget_position->item(3, 1)->text();
        } else {
            jsonDistance2["smallRange"] = ui->tableWidget_position->item(0, 1)->text();
            jsonDistance2["mediumRange"] = ui->tableWidget_position->item(1, 1)->text();
            jsonDistance2["largeRange"] = ui->tableWidget_position->item(2, 1)->text();
            jsonDistance2["1E10-1E13"] = ui->tableWidget_position->item(3, 1)->text();
        }

        jsonDistances["01"] = jsonDistance1;
        jsonDistances["02"] = jsonDistance2;
        jsonControl["Distances"] = jsonDistances;
        jsonObj["Control"] = jsonControl;

        file.open(QIODevice::WriteOnly | QIODevice::Text);
        jsonDoc.setObject(jsonObj);
        file.write(jsonDoc.toJson());
        file.close();
    }
}

void ControlWidget::closeEvent(QCloseEvent */*event*/)
{
    this->save();
}

void ControlWidget::enableUiStatus()
{
    bool enabled = controlHelper->connected();
    ui->pushButton_backward->setEnabled(enabled);
    ui->pushButton_zero->setEnabled(enabled);
    ui->pushButton_stop->setEnabled(enabled);
    ui->pushButton_absolute_position->setEnabled(enabled);
    ui->pushButton_relative_position->setEnabled(enabled);
    ui->pushButton_speed->setEnabled(enabled);
}

void ControlWidget::on_pushButton_start_clicked()
{
    if (!controlHelper->connected())
        return;

    if (this->property("enableAuto").toBool()){
        this->setProperty("enableAuto", false);
        ui->pushButton_start->setText(tr("开启往返运动"));

        controlHelper->single_stop(0x01);
        controlHelper->single_stop(0x02);
    }
    else{
//        controlHelper->single_moveabs(0x01, ui->doubleSpinBox_lower->value() * 1000);
//        controlHelper->single_moveabs(0x02, ui->doubleSpinBox_lower->value() * 1000);
        this->setProperty("enableAuto", true);
        ui->pushButton_start->setText(tr("停止往返运动"));

        controlHelper->single_jogleft(0x01);
        controlHelper->single_jogleft(0x02);
    }
}

void ControlWidget::on_pushButton_clicked()
{
    if (mAxis_no == 0x01)
        ui->doubleSpinBox_lower->setValue(ui->lineEdit_position->text().toFloat());
    else
        ui->doubleSpinBox_lower->setValue(ui->lineEdit_position_2->text().toFloat());
}

void ControlWidget::on_pushButton_2_clicked()
{
    if (mAxis_no == 0x01)
        ui->doubleSpinBox_upper->setValue(ui->lineEdit_position->text().toFloat());
    else
        ui->doubleSpinBox_upper->setValue(ui->lineEdit_position->text().toFloat());
}
