#ifndef OFFLINEDATAANALYSISWIDGET_H
#define OFFLINEDATAANALYSISWIDGET_H

#include <QWidget>
#include "sysutils.h"
#include "coincidenceanalyzer.h"

namespace Ui {
class OfflineDataAnalysisWidget;
}

class OfflineDataAnalysisWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OfflineDataAnalysisWidget(QWidget *parent = nullptr);
    ~OfflineDataAnalysisWidget();

    void initCustomPlot();
    void doEnWindowData(SingleSpectrum r1, vector<CoincidenceResult> r3);
    void openEnergyFile(QString filePath);

signals:
    void sigPlot(SingleSpectrum, vector<CoincidenceResult>, int refreshT);
    void sigStart();
    void sigPausePlot(bool); //是否暂停图像刷新
    void sigEnd(bool);

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void slotStart();
    void slotEnd(bool);

private slots:
    void on_pushButton_start_clicked();

private:
    Ui::OfflineDataAnalysisWidget *ui;
    QString currentFilename;
    CoincidenceAnalyzer* coincidenceAnalyzer = nullptr;
    quint32 stepT = 1;
    bool reAnalyzer = false; // 是否重新开始解析
    bool analyzerFinished = true;// 解析是否完成
};

#endif // OFFLINEDATAANALYSISWIDGET_H
