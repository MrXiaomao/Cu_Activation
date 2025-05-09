#pragma once
#include <QWidget>

class QImageProgressBar :
    public QWidget
{
public:
	explicit QImageProgressBar(QWidget *parent = nullptr);
	virtual ~QImageProgressBar();

	void setBackgroundImg(QString);
	void setBarImg(QString);

	virtual void paintEvent(QPaintEvent *paintEvent);

    quint64 minimum() const;
    quint64 maximum() const;

    quint64 value() const;

    virtual QString text() const;
    void setTextVisible(bool visible);
    bool isTextVisible() const;

public Q_SLOTS:
    void setRange(quint64 minimum, quint64 maximum);
    void setValue(quint64 value);

private:
	QPixmap mBarPixmap;
	QPixmap mBackPixmap;
    quint64 mValue = 0;
    quint64 mMinimum = 0;
    quint64 mMaximum = 100;
    bool mTextVisible;
};

