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

ControlWidget::ControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlWidget),
    currentAxiaName("01")
{
    ui->setupUi(this);

    QButtonGroup* grp = new QButtonGroup();
    grp->setExclusive(true);
    grp->addButton(ui->radioButton_01, 0);
    grp->addButton(ui->radioButton_02, 1);
    connect(grp, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [=](int index){
        this->save();
        if (0 == index){
            ui->groupBox->setTitle(tr("轴1"));
            currentAxiaName = "01";
        }
        else{
            ui->groupBox->setTitle(tr("轴2"));
            currentAxiaName = "02";
        }

        this->load();
        mCurrentHandle = mHandle[index];
        enableUiStatus();
        ui->comboBox_ip->setCurrentIndex(index);
        ui->comboBox_port->setCurrentIndex(index);
    });
    //emit grp->buttonClicked(0);

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

    QTimer *timer = new QTimer(this);
    timer->setObjectName("timerPosition");
    connect(timer, &QTimer::timeout, this, [=](){
        if (mCurrentHandle != 0){
            {
                float pos;
                uint32_t status;
                byte isrunning;
                byte limits[2];
                int ret = fti_single_getpos(mCurrentHandle, currentAxiaName.toStdString().c_str(), &pos);
                if (FT_SUCCESS == ret){
                    ui->lineEdit_position->setText(QString::number(pos / 1000, 'f', 4));
                }

                if (this->property("enableAuto").toBool()){
                    float realPos = pos / 1000;
                    if (realPos <= ui->doubleSpinBox_lower->value()){
                        fti_single_stop(mCurrentHandle, currentAxiaName.toStdString().c_str());
                        QTimer::singleShot(ui->spinBox_delay->value(), this, [=](){
                            fti_single_moveabs(mCurrentHandle, currentAxiaName.toStdString().c_str(), ui->doubleSpinBox_upper->value() * 1000);
                        });
                    } else if (realPos >= ui->doubleSpinBox_upper->value()){
                        fti_single_stop(mCurrentHandle, currentAxiaName.toStdString().c_str());
                        QTimer::singleShot(ui->spinBox_delay->value(), this, [=](){
                            fti_single_moveabs(mCurrentHandle, currentAxiaName.toStdString().c_str(), ui->doubleSpinBox_lower->value() * 1000);
                        });
                    }
                }
                ret = fti_single_getstatus(mCurrentHandle, currentAxiaName.toStdString().c_str(), &status);
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

                if (limits[0] == FT_TRUE){
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
        }        
    });

    this->load();

    //量程
    //负限位
    //正限位
    for (int i=0; i<=5; ++i){
        QTableWidget* tableWidgets[] = {ui->tableWidget_position};
        for (auto tableWidget : tableWidgets){
            tableWidget->setItem(i, 1, new QTableWidgetItem());
            tableWidget->item(i, 1)->setTextAlignment(Qt::AlignCenter);
            //tableWidget->item(row, 0)->setFlags(ui->tableWidget->item(row, 0)->flags() & (~Qt::ItemIsEditable));

            QWidget *w = new QWidget(tableWidget);
            w->setLayout(new QHBoxLayout());
            QPushButton *btn1 = new QPushButton(tr("设定"));
            QPushButton *btn2 = new QPushButton(tr("调用"));
            tableWidget->setCellWidget(i, 2, w);
            w->layout()->setContentsMargins(0,0,0,0);
            w->layout()->addWidget(btn1);
            w->layout()->addWidget(btn2);
            connect(btn1, &QPushButton::clicked, this, [&,tableWidget,i](){
                tableWidget->item(i, 1)->setText(ui->lineEdit_position->text());
            });
            connect(btn2, &QPushButton::clicked, this, [&,tableWidget,i](){
                float v = tableWidget->item(i, 1)->text().toFloat();
                if (mCurrentHandle == 0)
                    return ;

                if (i<=3) //量程
                    fti_single_moveabs(mCurrentHandle, currentAxiaName.toStdString().c_str(), v * 1000);
                else if (4==i)//设定负限位
                    fti_set_sw_p1(mCurrentHandle, currentAxiaName.toStdString().c_str(), v * 1000);
                else if (5==i)//设定正限位
                    fti_set_sw_p2(mCurrentHandle, currentAxiaName.toStdString().c_str(), v * 1000);
            });
            btn1->setProperty("row", i);
            btn2->setProperty("row", i);
        }
    }

    ui->pushButton_forward->installEventFilter(this);
    ui->pushButton_backward->installEventFilter(this);
}

ControlWidget::~ControlWidget()
{
    delete ui;
}

void ControlWidget::on_pushButton_connect_clicked()
{
    //连接
    if (mCurrentHandle != 0){
        QTimer* timerPosition = this->findChild<QTimer*>("timerPosition");
        timerPosition->stop();
        fti_close(mCurrentHandle);
        mCurrentHandle = 0;
        enableUiStatus();
        return;
    }

    {
        int ret = fti_open_tcp(ui->comboBox_ip->currentText().toStdString().c_str(),
                               ui->comboBox_port->currentText().toUInt(),
                               ui->checkBox_limit->isChecked() ? FT_TRUE : FT_FALSE, &mCurrentHandle);
        if (FT_SUCCESS == ret){
            //ret = fti_single_home(mHandle1, currentAxiaName.toStdString().c_str());


            float dValue;
            int iValue;
            ushort uValue;
            //设定负限位
            fti_get_sw_p1(mCurrentHandle, currentAxiaName.toStdString().c_str(), &dValue);
            ui->tableWidget_position->item(4, 1)->setText(QString::number(dValue / 1000, 'f', 4));
            //设定正限位
            fti_get_sw_p2(mCurrentHandle, currentAxiaName.toStdString().c_str(), &dValue);
            ui->tableWidget_position->item(5, 1)->setText(QString::number(dValue / 1000, 'f', 4));

            //运动速度
            fti_get_vel(mCurrentHandle, currentAxiaName.toStdString().c_str(), &dValue);
            ui->doubleSpinBox_speed->setValue(dValue);

            //细分 螺距 加速系数 减速系数
//            fti_get_div(mCurrentHandle, currentAxiaName.toStdString().c_str(), &iValue);
//            fti_get_pitch(mCurrentHandle, currentAxiaName.toStdString().c_str(), &dValue);
//            fti_get_accel(mCurrentHandle, currentAxiaName.toStdString().c_str(), &uValue);
//            fti_get_decel(mCurrentHandle, currentAxiaName.toStdString().c_str(), &uValue);

            QTimer* timerPosition = this->findChild<QTimer*>("timerPosition");
            timerPosition->start(100);
            enableUiStatus();
            if (currentAxiaName == "01")
                mHandle[0] = mCurrentHandle;
            else
                mHandle[1] = mCurrentHandle;
        } else {
            mCurrentHandle = 0;
            if (currentAxiaName == "01")
                mHandle[0] = 0;
            else
                mHandle[1] = 0;
            QMessageBox::information(this, tr("提示"), tr("设备连接失败！"));
        }
    }
}

void ControlWidget::on_pushButton_zero_clicked()
{
    //回零
    if (mCurrentHandle == 0)
        return ;

    fti_single_home(mCurrentHandle, currentAxiaName.toStdString().c_str());
}

void ControlWidget::on_pushButton_stop_clicked()
{
    //停止
    if (mCurrentHandle == 0)
        return ;

    fti_single_stop(mCurrentHandle, currentAxiaName.toStdString().c_str());
}

void ControlWidget::on_pushButton_absolute_position_clicked()
{
    //绝对位置
    if (mCurrentHandle == 0)
        return ;

    fti_single_moveabs(mCurrentHandle, currentAxiaName.toStdString().c_str(), ui->doubleSpinBox_absolute_position->value() * 1000);
}

void ControlWidget::on_pushButton_relative_position_clicked()
{
    //相对位置
    if (mCurrentHandle == 0)
        return ;

    fti_single_move(mCurrentHandle, currentAxiaName.toStdString().c_str(), ui->doubleSpinBox_relative_position->value() * 1000);
}

void ControlWidget::on_pushButton_speed_clicked()
{
    //速度    
    if (mCurrentHandle == 0)
        return ;

    fti_set_vel(mCurrentHandle, currentAxiaName.toStdString().c_str(), ui->doubleSpinBox_speed->value());
}

void ControlWidget::on_pushButton_setup_clicked()
{
    //参数配置
    if (mCurrentHandle == 0)
        return ;

    fti_save_params_permanently(mCurrentHandle, currentAxiaName.toStdString().c_str());
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
        QJsonObject jsonDisplacement1, jsonDisplacement2;

        ui->comboBox_ip->clear();
        ui->comboBox_port->clear();

        if (jsonObj.contains("Displacement")){
            QJsonArray jsonDisplacements = jsonObj["Displacement"].toArray();
            jsonDisplacement1 = jsonDisplacements.at(0).toObject();
            jsonDisplacement2 = jsonDisplacements.at(1).toObject();
            ui->comboBox_ip->addItem(jsonDisplacement1["ip"].toString());
            ui->comboBox_port->addItem(QString::number(jsonDisplacement1["port"].toInt()));
            ui->comboBox_ip->addItem(jsonDisplacement2["ip"].toString());
            ui->comboBox_port->addItem(QString::number(jsonDisplacement2["port"].toInt()));
            if (currentAxiaName == "01"){
                ui->tableWidget_position->item(0, 1)->setText(jsonDisplacement1["0-1E4"].toString());
                ui->tableWidget_position->item(1, 1)->setText(jsonDisplacement1["1E4-1E7"].toString());
                ui->tableWidget_position->item(2, 1)->setText(jsonDisplacement1["1E7-1E10"].toString());
                ui->tableWidget_position->item(3, 1)->setText(jsonDisplacement1["1E10-1E13"].toString());
            } else {
                ui->tableWidget_position->item(0, 1)->setText(jsonDisplacement2["0-1E4"].toString());
                ui->tableWidget_position->item(1, 1)->setText(jsonDisplacement2["1E4-1E7"].toString());
                ui->tableWidget_position->item(2, 1)->setText(jsonDisplacement2["1E7-1E10"].toString());
                ui->tableWidget_position->item(3, 1)->setText(jsonDisplacement2["1E10-1E13"].toString());
            }
        }
    }
}

bool ControlWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != this){
        if (event->type() == QEvent::MouseButtonPress){
            if (watched->inherits("QPushButton")){
                QPushButton* btn = qobject_cast<QPushButton*>(watched);
                if (btn->objectName() == "pushButton_forward" || btn->objectName() == "pushButton_forward_2"){
                    //正向
                    if (mCurrentHandle == 0)
                        return QWidget::eventFilter(watched, event);

                    fti_single_stop(mCurrentHandle, currentAxiaName.toStdString().c_str());
                    fti_single_jogright(mCurrentHandle, currentAxiaName.toStdString().c_str());
                } else if (btn->objectName() == "pushButton_backward" || btn->objectName() == "pushButton_backward_2"){
                    //负向
                    if (mCurrentHandle == 0)
                        return QWidget::eventFilter(watched, event);

                    fti_single_stop(mCurrentHandle, currentAxiaName.toStdString().c_str());
                    fti_single_jogleft(mCurrentHandle, currentAxiaName.toStdString().c_str());
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease){
            if (watched->inherits("QPushButton")){
                if (mCurrentHandle == 0)
                    return QWidget::eventFilter(watched, event);

                fti_single_stop(mCurrentHandle, currentAxiaName.toStdString().c_str());
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
        QJsonObject jsonDisplacement;
        if (jsonObj.contains("Displacement")){
            QJsonArray jsonDisplacements = jsonObj["Displacement"].toArray();
            QJsonObject jsonDisplacement1, jsonDisplacement2;
            jsonDisplacement1 = jsonDisplacements.at(0).toObject();
            jsonDisplacement2 = jsonDisplacements.at(1).toObject();
            if (currentAxiaName == "01"){
                jsonDisplacement1["0-1E4"] = ui->tableWidget_position->item(0, 1)->text();
                jsonDisplacement1["1E4-1E7"] = ui->tableWidget_position->item(1, 1)->text();
                jsonDisplacement1["1E7-1E10"] = ui->tableWidget_position->item(2, 1)->text();
                jsonDisplacement1["1E10-1E13"] = ui->tableWidget_position->item(3, 1)->text();
            } else {
                jsonDisplacement2["0-1E4"] = ui->tableWidget_position->item(0, 1)->text();
                jsonDisplacement2["1E4-1E7"] = ui->tableWidget_position->item(1, 1)->text();
                jsonDisplacement2["1E7-1E10"] = ui->tableWidget_position->item(2, 1)->text();
                jsonDisplacement2["1E10-1E13"] = ui->tableWidget_position->item(3, 1)->text();
            }

            jsonDisplacements.replace(0, jsonDisplacement1);
            jsonDisplacements.replace(1, jsonDisplacement2);
            jsonObj["Displacement"] = jsonDisplacements;
        }

        file.open(QIODevice::WriteOnly | QIODevice::Text);
        jsonDoc.setObject(jsonObj);
        file.write(jsonDoc.toJson());
        file.close();
    }
}

void ControlWidget::closeEvent(QCloseEvent */*event*/)
{
    for (int i=0; i<2; ++i){
        if (mHandle[i] != 0){
            fti_close(mHandle[i]);
            mHandle[i] = 0;
        }
    }

    this->save();
}

void ControlWidget::enableUiStatus()
{
    bool enabled = mCurrentHandle!=0;
    ui->pushButton_connect->setText(enabled ? tr("断开连接") : tr("连接"));
    ui->pushButton_backward->setEnabled(enabled);
    ui->pushButton_zero->setEnabled(enabled);
    ui->pushButton_stop->setEnabled(enabled);
    ui->pushButton_absolute_position->setEnabled(enabled);
    ui->pushButton_relative_position->setEnabled(enabled);
    ui->pushButton_speed->setEnabled(enabled);
}

void ControlWidget::on_pushButton_start_clicked()
{
    if (this->property("enableAuto").toBool()){
        ui->pushButton_start->setText(tr("开启往返运动"));
        this->setProperty("enableAuto", false);
    }
    else{
        ui->pushButton_start->setText(tr("停止往返运动"));
        this->setProperty("enableAuto", true);
    }
}

void ControlWidget::on_pushButton_clicked()
{
    ui->doubleSpinBox_lower->setValue(ui->lineEdit_position->text().toFloat());
}

void ControlWidget::on_pushButton_2_clicked()
{
    ui->doubleSpinBox_upper->setValue(ui->lineEdit_position->text().toFloat());
}

void ControlWidget::on_pushButton_backward_clicked()
{

}
