#include "cachedirconfigwidget.h"
#include "ui_cachedirconfigwidget.h"
#include <QFileDialog>
#include <QToolButton>
#include <QAction>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

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

        QString cacheDir = jsonObj["defaultCache"].toString();
        ui->lineEdit_path->setText(cacheDir);
        commandhelper->setDefaultCacheDir(cacheDir);
    } else {
        QString cacheDir = "./";
        ui->lineEdit_path->setText(cacheDir);
        commandhelper->setDefaultCacheDir(cacheDir);
    }
}

bool CacheDirConfigWidget::save()
{
    QString cacheDir = ui->lineEdit_path->text();

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

        //if (jsonObj.contains("defaultCache")){
            jsonObj["defaultCache"] = cacheDir;
        //} else {
        //    jsonObj.insert("defaultCache", QJsonValue(cacheDir));
        //}

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
