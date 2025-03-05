#pragma once

#include <QWidget>
#include "ui_QSingleSelectTimeWidget.h"

class QSingleSelectTimeWidget : public QWidget
{
	Q_OBJECT

public:
	QSingleSelectTimeWidget(QWidget *parent = Q_NULLPTR);
	~QSingleSelectTimeWidget();
	//时间标识
	enum ENUM_TimeMode
	{
		TimeMode_Hour, //小时
		TimeMode_Minute, //分钟
		TimeMode_Second, //秒钟
	};
public: //对外开放接口
	void SetCurrentShowTime(ENUM_TimeMode enumTime, int data); //设置：当前显示时间
	int GetCurrentContent(); //获取：最新时间内容
protected:
	virtual void paintEvent(QPaintEvent *);//重载--绘图事件--绘制背景图
	virtual void wheelEvent(QWheelEvent *event); //滚轮
	void OnBnClickedUp(); //上一个
	void OnBnClickedDown(); //下一个

signals:
    void signal_update_cur_time_val();
private:
	void InitUI();
	void LoadConnectMsg();
	void SetTimeChange(ENUM_TimeMode enumTime, int data); //根据枚举进行时间变化
private:
	Ui::QSingleSelectTimeWidget ui;
	ENUM_TimeMode m_enumTime; //时间模式
	int m_nCurrentData; //当前数值
};
