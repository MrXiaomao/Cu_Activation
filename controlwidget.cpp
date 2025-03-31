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
    ui(new Ui::ControlWidget),
    currentAxiaName("01")
{
    ui->setupUi(this);
    controlhelper = ControlHelper::instance();

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
        enableUiStatus();
        ui->comboBox_ip->setCurrentIndex(0);
        ui->comboBox_port->setCurrentIndex(0);
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
        if (mHandle != 0){
            {
                float pos;
                uint32_t status;
                byte isrunning;
                byte limits[2];
                int ret = fti_single_getpos(mHandle, currentAxiaName.toStdString().c_str(), &pos);
                if (FT_SUCCESS == ret){
                    ui->lineEdit_position->setText(QString::number(pos / 1000, 'f', 4));
                }

                if (this->property("enableAuto").toBool()){
                    float realPos = pos / 1000;
                    if (realPos <= ui->doubleSpinBox_lower->value()){
                        fti_single_stop(mHandle, currentAxiaName.toStdString().c_str());
                        QTimer::singleShot(ui->spinBox_delay->value(), this, [=](){
                            fti_single_moveabs(mHandle, currentAxiaName.toStdString().c_str(), ui->doubleSpinBox_upper->value() * 1000);
                        });
                    } else if (realPos >= ui->doubleSpinBox_upper->value()){
                        fti_single_stop(mHandle, currentAxiaName.toStdString().c_str());
                        QTimer::singleShot(ui->spinBox_delay->value(), this, [=](){
                            fti_single_moveabs(mHandle, currentAxiaName.toStdString().c_str(), ui->doubleSpinBox_lower->value() * 1000);
                        });
                    }
                }
                ret = fti_single_getstatus(mHandle, currentAxiaName.toStdString().c_str(), &status);
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
            QPushButton *btn2 = new QPushButton(tr("Go->"));
            tableWidget->setCellWidget(i, 2, w);
            w->layout()->setContentsMargins(0,0,0,0);
            w->layout()->addWidget(btn1);
            w->layout()->addWidget(btn2);
            connect(btn1, &QPushButton::clicked, this, [&,tableWidget,i](){
                tableWidget->item(i, 1)->setText(ui->lineEdit_position->text());
            });
            connect(btn2, &QPushButton::clicked, this, [&,tableWidget,i](){
                float v = tableWidget->item(i, 1)->text().toFloat();
                if (mHandle == 0)
                    return ;

                if (i<=3) //量程
                    fti_single_moveabs(mHandle, currentAxiaName.toStdString().c_str(), v * 1000);
                else if (4==i)//设定负限位
                    fti_set_sw_p1(mHandle, currentAxiaName.toStdString().c_str(), v * 1000);
                else if (5==i)//设定正限位
                    fti_set_sw_p2(mHandle, currentAxiaName.toStdString().c_str(), v * 1000);
            });
            btn1->setProperty("row", i);
            btn2->setProperty("row", i);
        }
    }

    ui->pushButton_forward->installEventFilter(this);
    ui->pushButton_backward->installEventFilter(this);

    this->load();
}

ControlWidget::~ControlWidget()
{
    delete ui;
}

//#include <QLibrary>
//extern "C" {
//    typedef __cdecl  int (*PFfti_open_tcp)(const char* ip, const int port, const byte limit_isnegative, FT_H* handle);
//    typedef const char* (*PFfti_getsdkversion)();
//}

void ControlWidget::on_pushButton_connect_clicked()
{
//    QLibrary myLib("ftcoreimc.dll"); // 指定要加载的 DLL 文件

//    if (myLib.load())
//    {
//        PFfti_getsdkversion fti_getsdkversion = (PFfti_getsdkversion)myLib.resolve("fti_getsdkversion");
//        if (fti_getsdkversion){
//            QMessageBox::information(this, "ver", QString::fromLocal8Bit(fti_getsdkversion()));
//        }
//        QMessageBox::information(this, "tip", "ftcoreimc.dll loaded successfully");
//    }

    //连接
    if (mHandle != 0){
        ui->pushButton_start->setText(tr("开启往返运动"));
        this->setProperty("enableAuto", false);
        fti_single_stop(mHandle, currentAxiaName.toStdString().c_str());
        QTimer* timerPosition = this->findChild<QTimer*>("timerPosition");
        timerPosition->stop();
        fti_close(mHandle);
        mHandle = 0;
        enableUiStatus();
        return;
    }

    {
//        QMessageBox::information(this, "tip", "3");
//        PFfti_open_tcp fti_open_tcp = (PFfti_open_tcp)myLib.resolve("fti_open_tcp");
//        if (fti_open_tcp)
//        {
//            QMessageBox::information(this, "tip", "34");
//            int ret = fti_open_tcp(ui->comboBox_ip->currentText().toStdString().c_str(), ui->comboBox_port->currentText().toUInt(), FT_TRUE, &mCurrentHandle);
//            QMessageBox::information(this, "tip", "35");
//        }
//        QMessageBox::information(this, "tip", "36");
        int ret = fti_open_tcp(ui->comboBox_ip->currentText().trimmed().toStdString().c_str(),
                               ui->comboBox_port->currentText().toUInt(),
                               ui->checkBox_limit->isChecked() ? FT_TRUE : FT_FALSE, &mHandle);
        if (FT_SUCCESS == ret){
            //ret = fti_single_home(mHandle1, currentAxiaName.toStdString().c_str());

            float dValue;
            int iValue;
            ushort uValue;
            //设定负限位
            fti_get_sw_p1(mHandle, currentAxiaName.toStdString().c_str(), &dValue);
            ui->tableWidget_position->item(4, 1)->setText(QString::number(dValue / 1000, 'f', 4));
            //设定正限位
            fti_get_sw_p2(mHandle, currentAxiaName.toStdString().c_str(), &dValue);
            ui->tableWidget_position->item(5, 1)->setText(QString::number(dValue / 1000, 'f', 4));

            //运动速度
            fti_get_vel(mHandle, currentAxiaName.toStdString().c_str(), &dValue);
            ui->doubleSpinBox_speed->setValue(dValue);

            //细分 螺距 加速系数 减速系数
//            fti_get_div(mCurrentHandle, currentAxiaName.toStdString().c_str(), &iValue);
//            fti_get_pitch(mCurrentHandle, currentAxiaName.toStdString().c_str(), &dValue);
//            fti_get_accel(mCurrentHandle, currentAxiaName.toStdString().c_str(), &uValue);
//            fti_get_decel(mCurrentHandle, currentAxiaName.toStdString().c_str(), &uValue);

            QTimer* timerPosition = this->findChild<QTimer*>("timerPosition");
            timerPosition->start(100);
            enableUiStatus();
        } else {
            mHandle = 0;
            QMessageBox::information(this, tr("提示"), tr("设备连接失败！"));
        }
    }
}

void ControlWidget::on_pushButton_zero_clicked()
{
    //回零
    if (mHandle == 0)
        return ;

    fti_single_home(mHandle, currentAxiaName.toStdString().c_str());
}

void ControlWidget::on_pushButton_stop_clicked()
{
    //停止
    if (mHandle == 0)
        return ;

    fti_single_stop(mHandle, currentAxiaName.toStdString().c_str());
}

void ControlWidget::on_pushButton_absolute_position_clicked()
{
    //绝对位置
    if (mHandle == 0)
        return ;

    fti_single_moveabs(mHandle, currentAxiaName.toStdString().c_str(), ui->doubleSpinBox_absolute_position->value() * 1000);
}

void ControlWidget::on_pushButton_relative_position_clicked()
{
    //相对位置
    if (mHandle == 0)
        return ;

    fti_single_move(mHandle, currentAxiaName.toStdString().c_str(), ui->doubleSpinBox_relative_position->value() * 1000);
}

void ControlWidget::on_pushButton_speed_clicked()
{
    //速度    
    if (mHandle == 0)
        return ;

    fti_set_vel(mHandle, currentAxiaName.toStdString().c_str(), ui->doubleSpinBox_speed->value());
}

void ControlWidget::on_pushButton_setup_clicked()
{
    //参数配置
    if (mHandle == 0)
        return ;

    fti_save_params_permanently(mHandle, currentAxiaName.toStdString().c_str());
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

        ui->comboBox_ip->clear();
        ui->comboBox_port->clear();

        if (jsonObj.contains("Control")){
            QJsonObject jsonControl = jsonObj["Control"].toObject();
            QJsonArray jsonDistances = jsonControl["Distances"].toArray();

            ui->comboBox_ip->addItem(jsonControl["ip"].toString());
            ui->comboBox_port->addItem(QString::number(jsonControl["port"].toInt()));
            if (jsonControl.contains("Distances")){
                QJsonObject jsonDistance1 = jsonControl["Distances"].toObject()["01"].toObject();
                QJsonObject jsonDistance2 = jsonControl["Distances"].toObject()["02"].toObject();
                if (currentAxiaName == "01"){
                    ui->tableWidget_position->item(0, 1)->setText(jsonDistance1["0-1E4"].toString());
                    ui->tableWidget_position->item(1, 1)->setText(jsonDistance1["1E4-1E7"].toString());
                    ui->tableWidget_position->item(2, 1)->setText(jsonDistance1["1E7-1E10"].toString());
                    ui->tableWidget_position->item(3, 1)->setText(jsonDistance1["1E10-1E13"].toString());
                } else {
                    ui->tableWidget_position->item(0, 1)->setText(jsonDistance2["0-1E4"].toString());
                    ui->tableWidget_position->item(1, 1)->setText(jsonDistance2["1E4-1E7"].toString());
                    ui->tableWidget_position->item(2, 1)->setText(jsonDistance2["1E7-1E10"].toString());
                    ui->tableWidget_position->item(3, 1)->setText(jsonDistance2["1E10-1E13"].toString());
                }
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
                    if (mHandle == 0)
                        return QWidget::eventFilter(watched, event);

                    fti_single_stop(mHandle, currentAxiaName.toStdString().c_str());
                    fti_single_jogright(mHandle, currentAxiaName.toStdString().c_str());
                } else if (btn->objectName() == "pushButton_backward" || btn->objectName() == "pushButton_backward_2"){
                    //负向
                    if (mHandle == 0)
                        return QWidget::eventFilter(watched, event);

                    fti_single_stop(mHandle, currentAxiaName.toStdString().c_str());
                    fti_single_jogleft(mHandle, currentAxiaName.toStdString().c_str());
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease){
            if (watched->inherits("QPushButton")){
                if (mHandle == 0)
                    return QWidget::eventFilter(watched, event);

                fti_single_stop(mHandle, currentAxiaName.toStdString().c_str());
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

        if (currentAxiaName == "01"){
            jsonDistance1["0-1E4"] = ui->tableWidget_position->item(0, 1)->text();
            jsonDistance1["1E4-1E7"] = ui->tableWidget_position->item(1, 1)->text();
            jsonDistance1["1E7-1E10"] = ui->tableWidget_position->item(2, 1)->text();
            jsonDistance1["1E10-1E13"] = ui->tableWidget_position->item(3, 1)->text();
        } else {
            jsonDistance2["0-1E4"] = ui->tableWidget_position->item(0, 1)->text();
            jsonDistance2["1E4-1E7"] = ui->tableWidget_position->item(1, 1)->text();
            jsonDistance2["1E7-1E10"] = ui->tableWidget_position->item(2, 1)->text();
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
    if (mHandle != 0){
        fti_close(mHandle);
        mHandle = 0;
    }

    this->save();
}

void ControlWidget::enableUiStatus()
{
    bool enabled = mHandle!=0;
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
    if (mHandle == 0)
        return;

    if (this->property("enableAuto").toBool()){
        fti_single_stop(mHandle, currentAxiaName.toStdString().c_str());
        ui->pushButton_start->setText(tr("开启往返运动"));
        this->setProperty("enableAuto", false);
    }
    else{
        fti_single_moveabs(mHandle, currentAxiaName.toStdString().c_str(), ui->doubleSpinBox_lower->value() * 1000);
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
