#include "QSingleSelectTimeWidget.h"
#include <QPainter>
#include <QWheelEvent>

QSingleSelectTimeWidget::QSingleSelectTimeWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	m_enumTime = TimeMode_Hour; //默认，小时制
	this->InitUI();
	this->LoadConnectMsg();
}

QSingleSelectTimeWidget::~QSingleSelectTimeWidget()
{
}

void QSingleSelectTimeWidget::SetCurrentShowTime(ENUM_TimeMode enumTime, int data)
{
	m_enumTime = enumTime;
	m_nCurrentData = data;
	this->SetTimeChange(enumTime, data);
}

int QSingleSelectTimeWidget::GetCurrentContent()
{
	return m_nCurrentData;
}

void QSingleSelectTimeWidget::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.fillRect(this->rect(), QColor(255, 255, 255));
}

void QSingleSelectTimeWidget::wheelEvent(QWheelEvent *event)
{
	QPoint ptDegrees = event->angleDelta();
	//当前 ptDegrees.y 大于0，滚轮放大，反之缩小
	if (ptDegrees.y() > 0)
	{
		//滚轮向上
		this->OnBnClickedUp();
	}
	else
	{
		//滚轮向下
		this->OnBnClickedDown();
	}
	QWidget::wheelEvent(event);
}

void QSingleSelectTimeWidget::OnBnClickedUp()
{
	//当前数值向上移动-1，获取上一个展示label的数值
	int nPreviousData = ui.labPrevious->text().toInt();
	m_nCurrentData = nPreviousData;
	//重新设置数据变化
	this->SetTimeChange(m_enumTime, m_nCurrentData);
}

void QSingleSelectTimeWidget::OnBnClickedDown()
{
	//当前数值向上移动-1，获取下一个展示label的数值
	int nNextData = ui.labNext->text().toInt();
	m_nCurrentData = nNextData;
	//重新设置数据变化
	this->SetTimeChange(m_enumTime, m_nCurrentData);
}

void QSingleSelectTimeWidget::InitUI()
{
	//<button>上一个
	QString qsBtnUpStyle = "QPushButton{ outline:none;}"
        "QPushButton{border-image:url(../../image/fx_shang_n.png);}"
        "QPushButton:hover{border-image:url(../../image/fx_shang_s.png);}"
        "QPushButton:pressed{border-image:url(../../image/fx_shang_p.png);}";
	ui.btnUp->setGeometry(20, 7, 19, 19);
	ui.btnUp->setStyleSheet(qsBtnUpStyle);
	ui.btnUp->setText("");

	//<button>下一个
	QString qsBtnDownStyle = "QPushButton{ outline:none;}"
        "QPushButton{border-image:url(../../image/fx_xia_n.png);}"
        "QPushButton:hover{border-image:url(../../image/fx_xia_s.png);}"
        "QPushButton:pressed{border-image:url(../../image/fx_xia_p.png);}";
	ui.btnDown->setGeometry(20, 173, 19, 19);
	ui.btnDown->setStyleSheet(qsBtnDownStyle);
	ui.btnDown->setText("");

	//<label>上一个时间
	QString qsPreviousStyle = "QLabel{font-family:Microsoft YaHei UI; font-size:16px;color:#999999;background-color: transparent;}";
	ui.labPrevious->setGeometry(0, 34, 60, 44);
	ui.labPrevious->setStyleSheet(qsPreviousStyle);
	ui.labPrevious->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	ui.labPrevious->setText("0");

	//<label>当前时间
	QString qsCurrent = "QLabel{font-family:Microsoft YaHei UI; font-size:16px; font:blod; color:#333333;background-color: transparent;}";
	ui.labCurrent->setGeometry(0, 78, 60, 44);
	ui.labCurrent->setStyleSheet(qsCurrent);
	ui.labCurrent->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	ui.labCurrent->setText("0");

	//<label>下一个时间
	QString qsNext = "QLabel{font-family:Microsoft YaHei UI; font-size:16px;color:#999999;background-color: transparent;}";
	ui.labNext->setGeometry(0, 122, 60, 44);
	ui.labNext->setStyleSheet(qsNext);
	ui.labNext->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	ui.labNext->setText("0");
}

void QSingleSelectTimeWidget::LoadConnectMsg()
{
	//<button>上一个
	connect(ui.btnUp, &QPushButton::clicked, this, &QSingleSelectTimeWidget::OnBnClickedUp);
	//<button>下一个
	connect(ui.btnDown, &QPushButton::clicked, this, &QSingleSelectTimeWidget::OnBnClickedDown);
}

void QSingleSelectTimeWidget::SetTimeChange(ENUM_TimeMode enumTime, int data)
{
	switch (enumTime)
	{
	case QSingleSelectTimeWidget::TimeMode_Hour: //小时制度
	{
		//区间范围：[0-24)

		//设置：当前时间
		ui.labCurrent->setText(QString::number(data));
		//设置：上一个时间
		int nPreviousData = data == 0 ? 23 : (data - 1);
		ui.labPrevious->setText(QString::number(nPreviousData));
		//设置：下一个时间
		int nNextData = data == 23 ? 0 : (data + 1);
		ui.labNext->setText(QString::number(nNextData));
        // 传递消息，更新时间
        emit signal_update_cur_time_val();
	}
	break;
	case QSingleSelectTimeWidget::TimeMode_Minute: //分钟制度
	case QSingleSelectTimeWidget::TimeMode_Second: //秒制度
	{
		//区间范围：[0-60)

		//设置：当前时间
		ui.labCurrent->setText(QString::number(data));
		//设置：上一个时间
		int nPreviousData = data == 0 ? 59 : (data - 1);
		ui.labPrevious->setText(QString::number(nPreviousData));
		//设置：下一个时间
		int nNextData = data == 59 ? 0 : (data + 1);
		ui.labNext->setText(QString::number(nNextData));
        // 传递消息，更新时间
        emit signal_update_cur_time_val();
	}
	break;
    }
}
