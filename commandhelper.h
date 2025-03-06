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
    void  sigPowerStatus(bool on);

    // 继电器状态
    void  sigRelayStatus(bool on1, bool on2);

public slots:
    void openPower(QString ip, qint32 port);
    void closePower(QString ip, qint32 port);

private:
    QTcpSocket *socket;

};

#endif // COMMANDHELPER_H
