#ifndef COMMANDHELPER_H
#define COMMANDHELPER_H

#include <QObject>
#include <QTcpSocket>

class CommandHelper : public QObject
{
    Q_OBJECT
public:
    explicit CommandHelper(QObject *parent = nullptr);

signals:
    // 电源状态
    void sigPowerStatus(bool on);

    // 继电器状态
    void sigRelayStatus(bool on1, bool on2);

private:
    QByteArray frame;
    QByteArray data;

protected:
    void makeFrame();
    quint8 crc();

public slots:
    void openPower(QString ip, qint32 port);
    void closePower(QString ip, qint32 port);

    //触发阈值1
    void slotTriggerThold1(quint16 ch1, quint16 ch2);

    //触发阈值2
    void slotTriggerThold2(quint16 ch3, quint16 ch4);

    //波形极性
    void slotWaveformPolarity(quint8 v = 0x00);

    //波形触发模式
    void slotWaveformTriggerMode(quint8 mode = 0x00);

    //波形长度
    void slotWaveformLength(quint8 v = 0x04);

    //能谱模式/粒子模式死时间
    void slotSpectnumMode(quint16 dieTimelength);

    //能谱刷新时间
    void slotSpectnumRefreshTimeLength(quint32 refreshTimelength);

    //探测器增益
    void slotDetectorGain(quint8 ch1, quint8 ch2, quint8 ch3, quint8 ch4);

    //传输模式
    void slotTransferMode(quint8 ch1, quint8 ch2, quint8 ch3, quint8 ch4, quint8 mode);

private:
    QTcpSocket *socket;

};

#endif // COMMANDHELPER_H
