#include "cachedirconfigwidget.h"
#include "ui_cachedirconfigwidget.h"
#include <QFileDialog>
#include <QToolButton>
#include <QAction>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include "offlinedataanalysiswidget.h"

CacheDirConfigWidget::CacheDirConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CacheDirConfigWidget)
{
    ui->setupUi(this);

    commandhelper = CommandHelper::instance();

    QAction *action = ui->lineEdit_path->addAction(QIcon(":/resource/open.png"), QLineEdit::TrailingPosition);
    QToolButton* button = qobject_cast<QToolButton*>(action->associatedWidgets().last());
    button->setCursor(QCursor(Qt::PointingHandCursor));
    connect(button, &QToolButton::pressed, this, [=](){
        QString cacheDir = QFileDialog::getExistingDirectory(this);
        if (!cacheDir.isEmpty()){
            ui->lineEdit_path->setText(cacheDir);
        }
    });

    this->load();
}

CacheDirConfigWidget::~CacheDirConfigWidget()
{
    delete ui;
}

void CacheDirConfigWidget::on_pushButton_ok_clicked()
{
    if (this->save())
        this->close();
}

void CacheDirConfigWidget::on_pushButton_cancel_clicked()
{
    this->close();
}

void CacheDirConfigWidget::load()
{
    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    QFile file(QApplication::applicationDirPath() + "/config/user.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close(); //释放资源

        // 将 JSON 数据解析为 QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        if (jsonObj.contains("defaultCache")){
            QString cacheDir = jsonObj["defaultCache"].toString();
            //判断路径是否有效，
            if(OfflineDataAnalysisWidget::isValidFilePath(cacheDir)){
                ui->lineEdit_path->setText(cacheDir);
                commandhelper->setDefaultCacheDir(cacheDir);
                return;
            }
        }

        //检测不到缓存路径，或者缓存路径是无效路径，则用默认路径
        QString exePath = QCoreApplication::applicationDirPath();

        //先确保cache存在
        QString path = exePath + "/cache";
        QDir dir(path);
        if (!dir.exists())
            dir.mkdir(path);

        QString cacheDir = exePath + "/cache";
        jsonObj["defaultCache"] = cacheDir;
        ui->lineEdit_path->setText(cacheDir);
        commandhelper->setDefaultCacheDir(cacheDir);
        
        //重新写回文件
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QJsonDocument jsonDocNew(jsonObj);
        file.write(jsonDocNew.toJson());
        file.close();
    }
}

bool CacheDirConfigWidget::save()
{
    QString cacheDir = ui->lineEdit_path->text();

    //判断路径是否有效，
    if(!OfflineDataAnalysisWidget::isValidFilePath(cacheDir)){
        QMessageBox::information(nullptr, tr("提示"), tr("路径不存在，请检查正确后重新保存！"));
        return false;
    }

    QString path = QApplication::applicationDirPath() + "/config";
    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);

    QFile file(QApplication::applicationDirPath() + "/config/user.json");
    if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        // 读取文件内容
        QByteArray jsonData = file.readAll();
        file.close();

        // 将 JSON 数据解析为 QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        jsonObj["defaultCache"] = cacheDir;

        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QJsonDocument jsonDocNew(jsonObj);
        file.write(jsonDocNew.toJson());
        file.close();

        commandhelper->setDefaultCacheDir(cacheDir);
        return true;
    } else {
        return false;
    }
}
