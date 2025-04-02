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
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
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

    this->close();
}
