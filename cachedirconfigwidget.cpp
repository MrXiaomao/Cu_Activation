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
#include "globalsettings.h"

QReadWriteLock* CacheDirConfigWidget::m_sLock = new QReadWriteLock; //为静态变量new出对象

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
    JsonSettings* userSettings = GlobalSettings::instance()->mUserSettings;
    if (userSettings->isOpen()){
        userSettings->prepare();
        userSettings->beginGroup();
        QString defaultCache = QCoreApplication::applicationDirPath() + "/cache";
        QDir dir(defaultCache);
        if (!dir.exists())
            dir.mkdir(defaultCache);

        QString cacheDir = userSettings->value("defaultCache", defaultCache).toString();
        //判断路径是否有效，
        if(OfflineDataAnalysisWidget::isValidFilePath(cacheDir)){
            ui->lineEdit_path->setText(cacheDir);
            commandhelper->setDefaultCacheDir(cacheDir);
        }
        else{
            ui->lineEdit_path->setText(defaultCache);
            commandhelper->setDefaultCacheDir(defaultCache);
        }
        userSettings->endGroup();
        userSettings->finish();
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

    JsonSettings* userSettings = GlobalSettings::instance()->mUserSettings;
    if (userSettings->isOpen()){
        userSettings->prepare();
        userSettings->beginGroup();
        userSettings->setValue("defaultCache", cacheDir);
        userSettings->endGroup();
        bool result = userSettings->flush();
        userSettings->finish();
        if (result){
            QMessageBox::information(nullptr, tr("提示"), tr("缓存路径设置成功！"));
            return true;
        }
        commandhelper->setDefaultCacheDir(cacheDir);
        return true;
    } else {
        return false;
    }
}
