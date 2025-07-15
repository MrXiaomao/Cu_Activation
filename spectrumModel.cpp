#include "spectrumModel.h"
#include "ui_spectrumModel.h"
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

SpectrumModel::SpectrumModel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SpectrumModel)
    , measuring(false)
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
        ui->label_5->setText(time.toString("HH:mm:ss"));
    });

    this->load();
    ui->pushButton_save->setEnabled(false);
    ui->pushButton_start->setEnabled(commandhelper->isConnected());

    // 步长+自动修正
    ui->spinBox_3->setToolTip("请输入10的倍数（如10, 20, 30）");
    ui->spinBox_3->setSingleStep(10);
    connect(ui->spinBox_3, QOverload<int>::of(&QSpinBox::valueChanged), [=](int value) {
        if (value % 10 != 0) {
            ui->spinBox_3->blockSignals(true);
            ui->spinBox_3->setValue((value / 10) * 10);
            ui->spinBox_3->blockSignals(false);
        }
    });

    connect(commandhelper, &CommandHelper::sigMeasureStart, this, [=](qint8 mmode, qint8 tmode){
        measuring = true;

        timerStart = QDateTime::currentDateTime();
        ui->label_2->setText(timerStart.toString("yyyy-MM-dd HH:mm:ss"));
        timer->start(500);
        ui->pushButton_start->setEnabled(false);
        ui->pushButton_save->setEnabled(false);
        ui->pushButton_stop->setEnabled(true);
    });

    connect(commandhelper, &CommandHelper::sigMeasureStopSpectrum, this, [=](){
        timer->stop();
        measuring = false;
        ui->pushButton_stop->setEnabled(false);
        ui->pushButton_save->setEnabled(true);
        ui->pushButton_start->setEnabled(true);
    });
}

SpectrumModel::~SpectrumModel()
{
    disconnect(commandhelper, nullptr, this, nullptr);
    if (measuring){
        commandhelper->slotStopManualMeasure();
    }

    delete ui;
}

void SpectrumModel::load()
{
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/Spectrum.json");
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

        ui->spinBox_4->setValue(jsonObj["RefreshTimeLength"].toInt());

        ui->lineEdit_path->setText(jsonObj["Path"].toString());
        ui->lineEdit_filename->setText(jsonObj["FileName"].toString());
    }
}

bool SpectrumModel::save()
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
        if (ui->comboBox_4->currentText() == "1.26"){
            ch1 = 0x04;
        } else if (ui->comboBox_4->currentText() == "2.52"){
            ch1 = 0x05;
        } else if (ui->comboBox_4->currentText() == "5.01"){
            ch1 = 0x06;
        } else {
            ch1 = 0x04;
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

    //死时间
    {
        quint16 deadTime = ui->spinBox_3->value();
        jsonObj["DeadTime"] = deadTime;
    }

    //能谱刷新时间
    {
        quint16 refreshTimeLength = ui->spinBox_4->value();
        jsonObj["RefreshTimeLength"] = refreshTimeLength;
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
    QFile file(QApplication::applicationDirPath() + "/config/spectrum.json");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument jsonDoc(jsonObj);
        file.write(jsonDoc.toJson());
        file.close();
    } else {
        return false;
    }

    return true;
}

void SpectrumModel::on_pushButton_start_clicked()
{
    QAbstractButton *btn = qobject_cast<QAbstractButton*>(sender());
    if (!measuring){
        // 先保存参数
        if (!this->save()){
            return;
        }

        //手动测量
        DetectorParameter detectorParameter;
        detectorParameter.triggerThold1 = 0x81;
        detectorParameter.triggerThold2 = 0x81;
        detectorParameter.waveformPolarity = 0x00;
        detectorParameter.refreshTimeLength = 0x01;
        detectorParameter.gain = 0x00;
        detectorParameter.transferModel = 0x00;// 0x00-能谱 0x03-波形 0x05-符合模式
        detectorParameter.measureModel = mmManual;
        
        // 默认打开梯形成形
        detectorParameter.isTrapShaping = true;
        detectorParameter.TrapShape_risePoint = 20;
        detectorParameter.TrapShape_peakPoint = 20;
        detectorParameter.TrapShape_fallPoint = 20;
        detectorParameter.TrapShape_constTime1 = (int16_t)0.9558*65536;
        detectorParameter.TrapShape_constTime2 = (int16_t)0.9556*65536;
        detectorParameter.Threshold_baseLine = 20;

        // 打开 JSON 文件
        QFile file(QApplication::applicationDirPath() + "/config/spectrum.json");
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
            detectorParameter.refreshTimeLength = jsonObj["RefreshTimeLength"].toInt();
            detectorParameter.gain = jsonObj["DetectorGain"].toInt();
        }

        commandhelper->slotStartManualMeasure(detectorParameter);

        ui->pushButton_save->setEnabled(false);
        ui->pushButton_start->setEnabled(false);
        ui->pushButton_stop->setEnabled(true);

        QTimer::singleShot(3000, this, [=](){
            //指定时间未收到开始测量指令，则按钮恢复初始状态
            if (!measuring){
                ui->pushButton_start->setEnabled(true);
            }
        });
    }
}

void SpectrumModel::on_pushButton_save_clicked()
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

        commandhelper->exportFile(path + "/" + filename);
    }
}

void SpectrumModel::closeEvent(QCloseEvent *event)
{
    if (measuring){
        QMessageBox::warning(this, tr("提示"), tr("测量过程中，禁止关闭该窗口！"), QMessageBox::Ok, QMessageBox::Ok);
        event->ignore();
        return ;
    }

    event->accept();
}

void SpectrumModel::on_pushButton_stop_clicked()
{
    commandhelper->slotStopManualMeasure();
}

