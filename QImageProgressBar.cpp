#include "QImageProgressBar.h"
#include <QPainter>
#include <QPainterPath>

QImageProgressBar::QImageProgressBar(QWidget *parent):
    QWidget(parent)
{

}

QImageProgressBar::~QImageProgressBar()
{
	
}

void QImageProgressBar::setBackgroundImg(QString strImagePath)
{
	this->mBackPixmap.load(strImagePath);
}

void QImageProgressBar::setBarImg(QString strImagePath)
{
	this->mBarPixmap.load(strImagePath);
}

void QImageProgressBar::paintEvent(QPaintEvent *paintEvent)
{
	Q_UNUSED(paintEvent);
	
    if (this->mBackPixmap.isNull() || this->mBackPixmap.isNull()){
        QWidget::paintEvent(paintEvent);
		return;
	}

	//双缓存图纸
	QPixmap pixmap(this->width(), this->height());
	//图纸透明
	pixmap.fill(QColor(0, 0, 0, 0));
	
	//画布
	QPainter painter(&pixmap);
	painter.setBackgroundMode(Qt::TransparentMode);

	//背景
	{
		QPixmap tempPixmap = this->mBackPixmap.copy(20, 0, 20, mBackPixmap.height());
		QPainterPath progressPath;
		progressPath.setFillRule(Qt::WindingFill);

		//移动到起点
		progressPath.moveTo(this->height() / 2, 0);
		//以90度起点，逆时针画270度
		progressPath.arcTo(QRect(0, 0, this->height(), this->height()), 90, 180);
		progressPath.lineTo(this->width() - (this->height() / 2), this->height());
		//以270度起点，逆时针画180度
		progressPath.arcTo(QRect(QPoint(this->width() - this->height(), 0), QSize(this->height(), this->height())), 270, 180);
		progressPath.lineTo(this->height() / 2, 0);

		//设置画刷
		painter.setBrush(QBrush(tempPixmap));
		//设置抗锯齿
        painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
		//画背景图
		painter.fillPath(progressPath, QBrush(tempPixmap));
	}

	//进度条
	{
		//进度条宽度
		int progressWidth = this->value() * this->width() / (this->maximum() - this->minimum());

		QPixmap tempPixmap = this->mBarPixmap.copy(20, 0, 20, mBarPixmap.height());
		QPainterPath progressPath;
		progressPath.setFillRule(Qt::WindingFill);

		//移动到起点
		progressPath.moveTo(this->height() / 2, 0);
		//以90度起点，逆时针画270度
		progressPath.arcTo(QRect(0, 0, this->height(), this->height()), 90, 180);
		progressPath.lineTo(progressWidth - (this->height() / 2), this->height());
		//以270度起点，逆时针画180度
		progressPath.arcTo(QRect(QPoint(progressWidth - this->height(), 0), QSize(this->height(), this->height())), 270, 180);
		progressPath.lineTo(this->height() / 2, 0);

		//设置画刷
		painter.setBrush(QBrush(tempPixmap));
		//设置抗锯齿
        painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
		//画背景图
		painter.fillPath(progressPath, QBrush(tempPixmap));
	}

	//文字
	if (this->isTextVisible()) {
		//计算百分比
        painter.setPen(Qt::white);
		painter.drawText(0, 0, this->width(), this->height(), Qt::AlignCenter, this->text());
	}

	//绘画完毕
	painter.end();

	//图纸拷贝
	QPainter painterThis(this);
	painterThis.drawPixmap(0, 0, pixmap);
}

quint64 QImageProgressBar::minimum() const
{
    return this->mMinimum;
}

quint64 QImageProgressBar::maximum() const
{
    return this->mMaximum;
}

quint64 QImageProgressBar::value() const
{
    return this->mValue;
}

QString QImageProgressBar::text() const
{
    return QString("%1%").arg(this->mValue * 100 / (this->mMaximum - this->mMinimum));
}

void QImageProgressBar::setTextVisible(bool visible)
{
    if (this->mTextVisible != visible){
        this->mTextVisible = visible;
        this->update();
    }
}

bool QImageProgressBar::isTextVisible() const
{
    return this->mTextVisible;
}

void QImageProgressBar::setRange(quint64 minimum, quint64 maximum)
{
    this->mMinimum = minimum;
    this->mMaximum = maximum;
}

void QImageProgressBar::setValue(quint64 value)
{
    if (value != this->mValue){
        this->mValue = value;
        this->update();
        //this->repaint();
    }
}
