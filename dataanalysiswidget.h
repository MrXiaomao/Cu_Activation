#ifndef DATAANALYSISWIDGET_H
#define DATAANALYSISWIDGET_H

#include <QWidget>

namespace Ui {
class DataAnalysisWidget;
}

class DataAnalysisWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataAnalysisWidget(QWidget *parent = nullptr);
    ~DataAnalysisWidget();

    void initCustomPlot();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void slotPlowWidgetDoubleClickEvent();

private:    
    Ui::DataAnalysisWidget *ui;
};

#endif // DATAANALYSISWIDGET_H
