#include "waveformmodel.h"
#include "ui_waveformmodel.h"
#include <QFileDialog>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QAction>
#include <QToolButton>
#include <QFileDialog>
#include <QElapsedTimer>

WaveformModel::WaveformModel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WaveformModel)
{
    ui->setupUi(this);

    commandhelper = CommandHelper::instance();

    //波形路径
    {
        QAction *action = ui->lineEdit_path->addAction(QIcon(":/resource/open.png"), QLineEdit::TrailingPosition);
        QToolButton* button = qobject_cast<QToolButton*>(action->associatedWidgets().last());
        button->setCursor(QCursor(Qt::PointingHandCursor));
        connect(button, &QToolButton::pressed, this, [=](){
            QString dir = QFileDialog::getExistingDirectory(this);
            ui->lineEdit_path->setText(dir);
        });
    }

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=](){
        QDateTime now = QDateTime::currentDateTime();
        qint64 tt = timerStart.secsTo(now);
        QTime time = QTime(0,0,0).addSecs(tt);
        ui->label_13->setText(time.toString("HH:mm:ss"));
    });

    connect(commandhelper, &CommandHelper::sigRecvDataSize, this, [=](qint32 size){
        auto cal = [=](qint64 sz){
            double num = (double)sz;
            QStringList list;
            list << "KB" << "MB" << "GB" << "TB";

            QStringListIterator i(list);
            QString unit("bytes");

            while(num >= 1024.0 && i.hasNext())
             {
                unit = i.next();
                num /= 1024.0;
            }

            return QString().setNum(num,'f',2)+" "+unit;
        };

        total_filesize += size;
        ui->label_size->setText(cal(total_filesize));
    });

    connect(commandhelper, &CommandHelper::sigRecvPkgCount, this, [=](qint32 count){
        static qint32 total_ref = 0;
        total_ref += count;
        ui->label_ref->setText(QString::number(total_ref));
    });

    connect(commandhelper, &CommandHelper::sigMeasureStart, this, [=](qint8 mmode, qint8 tmode){
        measuring = true;
        ui->pushButton_start->setText(tr("停止测量"));

        timerStart = QDateTime::currentDateTime();
        ui->label_7->setText(timerStart.toString("yyyy-MM-dd HH:mm:ss"));
        timer->start(500);
        ui->pushButton_start->setEnabled(true);
        ui->pushButton_save->setEnabled(false);
    });

    connect(commandhelper, &CommandHelper::sigMeasureStop, this, [=](){
        timer->stop();
        measuring = false;
        ui->pushButton_start->setText(tr("开始测量"));
        ui->pushButton_save->setEnabled(true);
        ui->pushButton_start->setEnabled(true);
    });

    this->load();
    ui->pushButton_save->setEnabled(false);
    ui->pushButton_start->setEnabled(commandhelper->isConnected());
}

WaveformModel::~WaveformModel()
{
    disconnect(commandhelper, nullptr, this, nullptr);
    if (measuring){
        commandhelper->slotStopManualMeasure();
    }

    delete ui;   
}

void WaveformModel::load()
{
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/wave.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        // 将 JSON 数据解析为 QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        ui->comboBox->setCurrentIndex(jsonObj["WaveformPolarity"].toInt());
        ui->comboBox_4->setCurrentIndex(jsonObj["DetectorGain"].toInt());

        ui->spinBox->setValue(jsonObj["TriggerThold1"].toInt());
        ui->spinBox_2->setValue(jsonObj["TriggerThold2"].toInt());

        ui->lineEdit_path->setText(jsonObj["Path"].toString());
        ui->lineEdit_filename->setText(jsonObj["FileName"].toString());

        ui->comboBox_3->setCurrentIndex(jsonObj["WaveLength"].toInt());
    }
}

bool WaveformModel::save()
{
    // 保存参数
    QJsonObject jsonObj;

    //波形极性
    quint8 v = ui->comboBox->currentIndex();
    jsonObj["WaveformPolarity"] = v;

    //探测器增益
    {
        /*
        十六进制	增益
        00	0.08
        01	0.16
        02	0.32
        03	0.63
        04	1.26
        05	2.52
        06	5.01
        07	10
        */
        quint8 ch1 = 0x00;
        if (ui->comboBox_4->currentText() == "0.08"){
            ch1 = 0x00;
        } else if (ui->comboBox_4->currentText() == "0.16"){
            ch1 = 0x01;
        } else if (ui->comboBox_4->currentText() == "0.32"){
            ch1 = 0x02;
        } else if (ui->comboBox_4->currentText() == "0.63"){
            ch1 = 0x03;
        } else if (ui->comboBox_4->currentText() == "1.26"){
            ch1 = 0x04;
        } else if (ui->comboBox_4->currentText() == "2.52"){
            ch1 = 0x05;
        } else if (ui->comboBox_4->currentText() == "5.01"){
            ch1 = 0x06;
        } else if (ui->comboBox_4->currentText() == "10"){
            ch1 = 0x07;
        } else {
            ch1 = 0x00;
        }
        jsonObj["DetectorGain"] = ch1;
    }

    //探测器1-2阈值
    {
        quint16 ch1 = (quint16)ui->spinBox->value();
        quint16 ch2 = (quint16)ui->spinBox_2->value();
        jsonObj["TriggerThold1"] = ch1;
        jsonObj["TriggerThold2"] = ch2;
    }

    //探测器3-4阈值
    {
        quint16 ch3 = 0x00;
        quint16 ch4 = 0x00;
        jsonObj["TriggerThold3"] = ch3;
        jsonObj["TriggerThold4"] = ch4;
    }

    //波形长度
    {
        quint8 waveLength = 0x00;
        if (ui->comboBox_3->currentText() == "64"){
            waveLength = 0x00;
        } else if (ui->comboBox_3->currentText() == "128"){
            waveLength = 0x01;
        } else if (ui->comboBox_3->currentText() == "256"){
            waveLength = 0x02;
        } else if (ui->comboBox_3->currentText() == "512"){
            waveLength = 0x03;
        } else if (ui->comboBox_3->currentText() == "1024"){
            waveLength = 0x04;
        } else{
            waveLength = 0x04;
        }
        jsonObj["WaveLength"] = waveLength;
    }

    //路径
    {
        jsonObj["Path"] = ui->lineEdit_path->text();
    }

    //文件名
    {
        jsonObj["FileName"] = ui->lineEdit_filename->text();
    }


    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/wave.json");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument jsonDoc(jsonObj);
        file.write(jsonDoc.toJson());
        file.close();
    } else {
        return false;
    }

    return true;
}

void WaveformModel::on_pushButton_start_clicked()
{
    QAbstractButton *btn = qobject_cast<QAbstractButton*>(sender());
    if (!measuring){
        // 先保存参数
        if (!this->save()){
            measuring = !measuring;
            return;
        }

        //手动测量
        DetectorParameter detectorParameter;
        detectorParameter.triggerThold1 = 0x81;
        detectorParameter.triggerThold2 = 0x81;
        detectorParameter.waveformPolarity = 0x00;
        detectorParameter.deadTime = 0x05 * 10;
        detectorParameter.gain = 0x00;
        detectorParameter.waveLength = 0x01;
        detectorParameter.transferModel = 0x03;// 0x00-能谱 0x03-波形 0x05-符合模式
        detectorParameter.triggerModel = 0x00;
        detectorParameter.measureModel = 0x02;

        // 打开 JSON 文件
        QFile file(QApplication::applicationDirPath() + "/config/wave.json");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // 读取文件内容
            QByteArray jsonData = file.readAll();
            file.close(); //释放资源

            // 将 JSON 数据解析为 QJsonDocument
            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
            QJsonObject jsonObj = jsonDoc.object();
            detectorParameter.triggerThold1 = jsonObj["TriggerThold1"].toInt();
            detectorParameter.triggerThold2 = jsonObj["TriggerThold2"].toInt();
            detectorParameter.waveformPolarity = jsonObj["WaveformPolarity"].toInt();
            detectorParameter.deadTime = jsonObj["DeadTime"].toInt();
            detectorParameter.waveLength = jsonObj["WaveLength"].toInt();
            detectorParameter.triggerModel = jsonObj["TriggerModel"].toInt();
            detectorParameter.gain = jsonObj["DetectorGain"].toInt();
        }

        total_filesize = 0;
        ui->label_size->setText("0 bytes");
        commandhelper->slotStartManualMeasure(detectorParameter);

        ui->pushButton_save->setEnabled(false);
        ui->pushButton_start->setEnabled(false);

        QTimer::singleShot(3000, this, [=](){
            //指定时间未收到开始测量指令，则按钮恢复初始状态
            if (!measuring){
                ui->pushButton_start->setEnabled(true);
            }
        });
    } else {
        ui->pushButton_save->setEnabled(false);
        ui->pushButton_start->setEnabled(false);
        commandhelper->slotStopManualMeasure();

        QTimer::singleShot(5000, this, [=](){
            //指定时间未收到停止测量指令，则按钮恢复初始状态
            if (measuring){
                //测试
                measuring = false;
                ui->pushButton_start->setText("开始测量");

                ui->pushButton_start->setEnabled(true);
            }
        });
    }
}

void WaveformModel::on_pushButton_save_clicked()
{
    //存储路径
    //存储文件名
    QString filename = ui->lineEdit_path->text() + "/" + ui->lineEdit_filename->text();
    QFileInfo fInfo(filename);
    if (fInfo.exists()){
        if (QMessageBox::question(this, tr("提示"), tr("保存文件名已经存在，是否覆盖重写？"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
            return ;
    }

    // 保存参数
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(dir.absolutePath());
    QFile file(QApplication::applicationDirPath() + "/config/wave.json");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QString path, filename;
        path = ui->lineEdit_path->text();
        filename = ui->lineEdit_filename->text();
        if (!filename.endsWith(".dat"))
            filename += ".dat";

        QJsonObject jsonObj;
        jsonObj["Path"] = path;
        jsonObj["FileName"] = filename;
        QJsonDocument jsonDoc(jsonObj);
        file.write(jsonDoc.toJson());
        file.close();

        commandhelper->saveFileName(path + "/" + filename);
    }
}

void WaveformModel::closeEvent(QCloseEvent *event)
{
    if (measuring){
        QMessageBox::warning(this, tr("提示"), tr("测量过程中，禁止关闭该窗口！"), QMessageBox::Ok, QMessageBox::Ok);
        event->ignore();
        return ;
    }

    event->accept();
}
