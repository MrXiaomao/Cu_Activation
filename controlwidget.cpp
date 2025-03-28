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

ControlWidget::ControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlWidget)
{
    ui->setupUi(this);

    // 打印SDK版本
    ui->tableWidget_position->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_position_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();		//获取设备的串口列表
    for (int i = 0; i < ports.size();i++)
    {
        for (int j = i + 1; j < ports.size(); j++)
        {
            QString name = ports.at(i).portName();	//i的串口数字
            int portNumI = name.mid(3).toInt();

            name = ports.at(j).portName();			//j的串口数字
            int portNumJ = name.mid(3).toInt();

            if (portNumI > portNumJ)				//ij交换
            {
                ports.swap(i, j);
            }
        }
    }

    for (auto port : ports)
    {
        ui->comboBox_com->addItem(port.portName());
        ui->comboBox_com_2->addItem(port.portName());
    }

    QTimer *timer = new QTimer(this);
    timer->setObjectName("timerPosition");
    connect(timer, &QTimer::timeout, this, [=](){
        if (mHandle1 != 0){
            {
                float pos;
                uint32_t status;
                byte isrunning;
                byte limits[2];
                int ret = fti_single_getpos(mHandle1, "01", &pos);
                if (FT_SUCCESS == ret){
                    ui->lineEdit_position->setText(QString::number(pos / 1000, 'f', 4));
                }

                ret = fti_single_getstatus(mHandle1, "01", &status);
                ret = fti_single_isrunning(status, &isrunning);
                ret = fti_single_getlimits(status, limits);
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

                if (limits[1] == FT_TRUE){
                    ui->pushButton_forward->setEnable(false);
                    ui->label_positive->setStyleSheet("min-width:16px;"
                                                      "min-height:16px;"
                                                      "max-width:16px;"
                                                      "max-height:16px;"
                                                      "border-radius: 8px;"
                                                      "border:1px solid rgb(200, 200, 200);"
                                                      "background-color: rgb(170, 255, 0);");
                } else {
                    ui->pushButton_forward->setEnable(true);
                    ui->label_positive->setStyleSheet("min-width:16px;"
                                                      "min-height:16px;"
                                                      "max-width:16px;"
                                                      "max-height:16px;"
                                                      "border-radius: 8px;"
                                                      "border:1px solid rgb(200, 200, 200);"
                                                      "background-color: rgb(96, 96, 96);");
                }

                if (limits[0] == FT_TRUE){
                    ui->pushButton_backward->setEnable(true);
                    ui->label_negative->setStyleSheet("min-width:16px;"
                                                      "min-height:16px;"
                                                      "max-width:16px;"
                                                      "max-height:16px;"
                                                      "border-radius: 8px;"
                                                      "border:1px solid rgb(200, 200, 200);"
                                                      "background-color: rgb(170, 255, 0);");
                } else {
                    ui->pushButton_backward->setEnable(true);
                    ui->label_negative->setStyleSheet("min-width:16px;"
                                                      "min-height:16px;"
                                                      "max-width:16px;"
                                                      "max-height:16px;"
                                                      "border-radius: 8px;"
                                                      "border:1px solid rgb(200, 200, 200);"
                                                      "background-color: rgb(96, 96, 96);");
                }
            }
        }

        if (mHandle2 != 0){

        }
    });

    this->load();
    ui->pushButton_forward->installEventFilter(this);
    ui->pushButton_forward_2->installEventFilter(this);
    ui->pushButton_backward->installEventFilter(this);
    ui->pushButton_backward_2->installEventFilter(this);
}

ControlWidget::~ControlWidget()
{
    delete ui;
}

void ControlWidget::on_pushButton_connect_clicked()
{
    //连接
    if (mHandle1 != 0){
        QTimer* timerPosition = this->findChild<QTimer*>("timerPosition");
        timerPosition->stop();
        fti_close(mHandle1);
        mHandle1 = 0;
        ui->pushButton_connect->setText(tr("连接"));
        ui->pushButton_setup->setEnabled(false);
        ui->pushButton_backward->setEnabled(false);
        ui->pushButton_zero->setEnabled(false);
        ui->pushButton_stop->setEnabled(false);
        ui->pushButton_absolute_position->setEnabled(false);
        ui->pushButton_relative_position->setEnabled(false);
        ui->pushButton_speed->setEnabled(false);
        return;
    }

    if (ui->radioButton_com->isChecked()){
        int ret = fti_open_com(ui->comboBox_com->currentText().toStdString().c_str(),
                               ui->comboBox_baudrate->currentText().toUInt(),
                               ui->checkBox_limit->isChecked() ? FT_TRUE : FT_FALSE, &mHandle1);
        if (FT_SUCCESS == ret){
            //ret = fti_single_home(mHandle1, "01");

            // 细分
            // 螺距
            // 脉冲当量
            // 加速系数
            // 减速系数

            //绝对位置
            float pos;
            int ret = fti_single_getpos(mHandle1, "01", &pos);
            if (FT_SUCCESS == ret){
                ui->doubleSpinBox_absolute_position->setValue(pos / 1000);
            }

            QTimer* timerPosition = this->findChild<QTimer*>("timerPosition");
            timerPosition->start(100);
            ui->pushButton_connect->setText(tr("断开连接"));
            ui->pushButton_setup->setEnabled(true);
            ui->pushButton_backward->setEnabled(true);
            ui->pushButton_zero->setEnabled(true);
            ui->pushButton_stop->setEnabled(true);
            ui->pushButton_absolute_position->setEnabled(true);
            ui->pushButton_relative_position->setEnabled(true);
            ui->pushButton_speed->setEnabled(true);
        } else {
            mHandle1 = 0;
            QMessageBox::information(this, tr("提示"), tr("串口打开失败！"));
        }
    } else {
        int ret = fti_open_tcp(ui->lineEdit_ip->text().toStdString().c_str(),
                               ui->lineEdit_port->text().toUInt(),
                               ui->checkBox_limit->isChecked() ? FT_TRUE : FT_FALSE, &mHandle1);
        if (FT_SUCCESS == ret){
            //ret = fti_single_home(mHandle1, "01");
            QTimer* timerPosition = this->findChild<QTimer*>("timerPosition");
            timerPosition->start(100);
            ui->pushButton_connect->setText(tr("断开连接"));
        } else {
            mHandle1 = 0;
            QMessageBox::information(this, tr("提示"), tr("设备连接失败！"));
        }
    }
}

void ControlWidget::on_pushButton_zero_clicked()
{
    //回零
    if (mHandle1 == 0)
        return ;

    fti_single_home(mHandle1, "01");
}

void ControlWidget::on_pushButton_stop_clicked()
{
    //停止
    if (mHandle1 == 0)
        return ;

    fti_single_stop(mHandle1, "01");
}

void ControlWidget::on_pushButton_absolute_position_clicked()
{
    //绝对位置
    if (mHandle1 == 0)
        return ;

    fti_single_moveabs(mHandle1, "01", ui->doubleSpinBox_absolute_position->value() * 1000);
}

void ControlWidget::on_pushButton_relative_position_clicked()
{
    //相对位置
    if (mHandle1 == 0)
        return ;

    fti_single_move(mHandle1, "01", ui->doubleSpinBox_relative_position->value() * 1000);
}

void ControlWidget::on_pushButton_speed_clicked()
{
    //速度    
    if (mHandle1 == 0)
        return ;

    fti_set_vel(mHandle1, "01", ui->doubleSpinBox_speed->value());
}

void ControlWidget::on_pushButton_setup_clicked()
{
    //参数配置
    if (mHandle1 == 0)
        return ;

    fti_save_params_permanently(mHandle1, "01");
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
        QJsonObject jsonDisplacement;

        if (jsonObj.contains("Displacement")){
            QJsonArray jsonDisplacements = jsonObj["Displacement"].toArray();
            jsonDisplacement = jsonDisplacements.at(0).toObject();
            ui->lineEdit_ip->setText(jsonDisplacement["ip"].toString());
            ui->lineEdit_port->setText(QString::number(jsonDisplacement["port"].toInt()));

            jsonDisplacement = jsonDisplacements.at(1).toObject();
            ui->lineEdit_ip_2->setText(jsonDisplacement["ip"].toString());
            ui->lineEdit_port_2->setText(QString::number(jsonDisplacement["port"].toInt()));
        }
    }
}

bool ControlWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != this){
        if (event->type() == QEvent::MouseButtonPress){
            QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
            if (watched->inherits("QPushButton")){
                QPushButton* btn = qobject_cast<QPushButton*>(watched);
                if (btn->objectName() == "pushButton_forward" || btn->objectName() == "pushButton_forward_2"){
                    //正向
                    if (mHandle1 == 0)
                        return QWidget::eventFilter(watched, event);

                    fti_single_stop(mHandle1, "01");
                    fti_single_jogright(mHandle1, "01");
                } else if (btn->objectName() == "pushButton_backward" || btn->objectName() == "pushButton_backward_2"){
                    //负向
                    if (mHandle1 == 0)
                        return QWidget::eventFilter(watched, event);

                    fti_single_stop(mHandle1, "01");
                    fti_single_jogleft(mHandle1, "01");
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease){
            QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
            if (watched->inherits("QPushButton")){
                if (mHandle1 == 0)
                    return QWidget::eventFilter(watched, event);

                fti_single_stop(mHandle1, "01");
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
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonObject jsonDisplacement;
        if (jsonObj.contains("Displacement")){
            QJsonArray jsonDisplacements = jsonObj["Displacement"].toArray();
            jsonDisplacement = jsonDisplacements.at(0).toObject();
            jsonDisplacement["ip"] = ui->lineEdit_ip->text();
            jsonDisplacement["port"] = ui->lineEdit_port->text().toInt();
            jsonDisplacement["0-1E4"] = ui->tableWidget_position->item(0, 1)->text();
            jsonDisplacement["1E4-1E7"] = ui->tableWidget_position->item(1, 1)->text().toInt();
            jsonDisplacement["1E7-1E10"] = ui->tableWidget_position->item(2, 1)->text().toInt();
            jsonDisplacement["1E10-1E14"] = ui->tableWidget_position->item(3, 1)->text().toInt();

            jsonDisplacement = jsonDisplacements.at(1).toObject();
            jsonDisplacement["0-1E4"] = ui->tableWidget_position_2->item(0, 1)->text();
            jsonDisplacement["1E4-1E7"] = ui->tableWidget_position_2->item(1, 1)->text().toInt();
            jsonDisplacement["1E7-1E10"] = ui->tableWidget_position_2->item(2, 1)->text().toInt();
            jsonDisplacement["1E10-1E14"] = ui->tableWidget_position_2->item(3, 1)->text().toInt();
        }

        if (!jsonObj.contains("Displacement")){
            jsonObj.insert("Displacement", jsonDisplacement);
        }
    }

    QJsonObject jsonObj;

    QJsonObject jsonPostion1;

    //轴1量程
    jsonDetector["0-1E4"] = ui->tableWidget_position->item(0, 1)->text();
    jsonDetector["1E4-1E7"] = ui->tableWidget_position->item(1, 1)->text().toInt();
    jsonDetector["1E7-1E10"] = ui->tableWidget_position->item(2, 1)->text().toInt();
    jsonDetector["1E10-1E14"] = ui->tableWidget_position->item(3, 1)->text().toInt();
    jsonObj.insert("Detector", jsonDetector);

    jsonRelay["ip"] = ui->tableWidget->item(1, 0)->text();
    jsonRelay["port"] = ui->tableWidget->item(1, 1)->text().toInt();
    jsonObj.insert("Relay", jsonRelay);

    QJsonObject jsonDisplacement1, jsonDisplacement2;
    jsonDisplacement1["ip"] = ui->tableWidget->item(2, 0)->text();
    jsonDisplacement1["port"] = ui->tableWidget->item(2, 1)->text().toInt();
    jsonDisplacement2["ip"] = ui->tableWidget->item(3, 0)->text();
    jsonDisplacement2["port"] = ui->tableWidget->item(3, 1)->text().toInt();

    QJsonArray jsonDisplacements;
    jsonDisplacements.append(jsonDisplacement1);
    jsonDisplacements.append(jsonDisplacement2);
    jsonObj.insert("Displacement", jsonDisplacements);

    //轴2量程

    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/ip.json");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument jsonDoc(jsonObj);
        file.write(jsonDoc.toJson());
        file.close();
    }
}
