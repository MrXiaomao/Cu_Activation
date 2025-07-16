#include "splashwidget.h"
#include "ui_splashwidget.h"
#include <QScreen>
#include <QTimer>

SplashWidget::SplashWidget(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SplashWidget)
{
    ui->setupUi(this);

    QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();
    int x = (screenRect.width() - width()) / 2;
    int y = (screenRect.height() - height()) / 2;
    this->move(x, y);

    // progressBar = new QImageProgressBar(this);
    // progressBar->setGeometry(680, 465, 300, 25);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setTextVisible(true);
    ui->progressBar->setBackgroundImg(QString::fromLocal8Bit(":/resource/ProgressBar.png"));
    ui->progressBar->setBarImg(QString::fromLocal8Bit(":/resource/ProgressSlider.png"));
    ui->progressBar->setValue(0);
    ui->progressBar->hide();

    connect(this, SIGNAL(sigUpdataProgress(unsigned long long,unsigned long long)), this, SLOT(slotUpdataProgress(unsigned long long,unsigned long long)), Qt::AutoConnection);

    //关闭倒计时定时器
    QTimer *timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, [=](){
        if (timeout == 0) {
            return;
        }

        if (currentSec < timeout) {
            currentSec++;
        } else {
            this->close();
        }
    });
    timer->start();
    //this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool); //此种用法会遮挡第3方应用程序窗口
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    this->raise();
}

SplashWidget::~SplashWidget()
{
    delete ui;
}

void SplashWidget::closeEvent(QCloseEvent *)
{
    timeout = 0;
}

void SplashWidget::setInfo(const QString &info, int fontSizeMain, int timeout)
{
    this->timeout = timeout;
    this->currentSec = 0;
    //关闭进度条
    ui->progressBar->hide();

    QFont font;
    font.setPixelSize(fontSizeMain);
    ui->label_Info->setFont(font);
    ui->label_Info->setText(info);
}

void SplashWidget::setInfo(const QString &info, bool allowClose, int fontSizeMain, int timeout)
{
    //关闭进度条
    ui->progressBar->hide();
    if (allowClose)
        ui->pushButton_cancel->show();
    else
        ui->pushButton_cancel->hide();

    setInfo(info, fontSizeMain, timeout);
}

void SplashWidget::setInfo(const QString &info, bool allowClose, bool showProgress, int fontSizeMain, int timeout)
{
    if (showProgress)
        ui->progressBar->show();
    else
        ui->progressBar->hide();

    if (allowClose)
        ui->pushButton_cancel->show();
    else
        ui->pushButton_cancel->hide();

    this->timeout = timeout;
    this->currentSec = 0;

    QFont font;
    font.setPixelSize(fontSizeMain);
    ui->label_Info->setFont(font);
    ui->label_Info->setText(info);
}

#include "controlhelper.h"
void SplashWidget::on_pushButton_cancel_clicked()
{
    ControlHelper::instance()->single_stop(0x01);
    ControlHelper::instance()->single_stop(0x02);

    emit sigCancel();
    this->close();    
}

void SplashWidget::updataProgress(unsigned long long value, unsigned long long maximum)
{
    ui->progressBar->setRange(0, maximum);
    ui->progressBar->setValue(value);
}

void SplashWidget::slotUpdataProgress(unsigned long long value, unsigned long long maximum)
{
    updataProgress(value, maximum);
}
