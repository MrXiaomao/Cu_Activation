#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QScreen>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QScreen>
#include <QSplashScreen>

QString QtMsgTypeToString(QtMsgType type)
{
    switch (type) {
    case QtInfoMsg:
        return "[info]";     break;
    case QtDebugMsg:
        return "[debug]";    break;
    case QtWarningMsg:
        return "[warning]";  break;
    case QtCriticalMsg:
        return "[critical]"; break;
    case QtFatalMsg:
        return "[fatal]";    break;
    default:
        return "Unknown";
    }
}

void LoggingHandler(QtMsgType type, const QMessageLogContext& /*context*/, const QString &msg)
{
    // 确保logs目录存在
    QDir dir(QDir::currentPath() + "/logs");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    //过滤掉警告日志信息
    if (type == QtWarningMsg)
        return;

    // 获取当前日期，并格式化为YYYY-MM-DD
    QString currentDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    // 创建日志文件路径，例如：logs/2023-10-23.log
    QString logFilePath = QDir::currentPath() + "/logs/Cu_Activation_" + currentDate + ".log";

    // 打开文件以追加模式
    QFile file(logFilePath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        QString logLine = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz ")
                          + QtMsgTypeToString(type) + ": " + msg + "\n";
        out << logLine;
        file.close();
    }
}

// 清理旧的日志文件，只保留最近几次的
void cleanOldLogs(int refsToKeep)
{
    QDir dir(QDir::currentPath() + "/logs");
    QStringList entries = dir.entryList(QStringList() << "Cu_Activation_*.log", QDir::Files);

    // 按文件名排序
    entries.sort();

    // 如果文件数量超过限制，则删除最旧的文件
    while (entries.size() > refsToKeep) {
        QString oldestFile = entries.first(); // 获取最旧的文件名
        QFile::remove(dir.filePath(oldestFile)); // 删除文件
        entries.removeFirst(); // 从列表中移除文件名
    }
}


int main(int argc, char *argv[])
{
    QApplication::setStyle(QStyleFactory::create("fusion"));//WindowsVista fusion windows
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling); // 启用高DPI缩放支持
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps); // 使用高DPI位图
    QApplication a(argc, argv);

    qDebug() << APP_VERSION;
    qDebug() << GIT_BRANCH;

    QSplashScreen splash;
    splash.setPixmap(QPixmap(":/resource/splash.png"));
    splash.show();

    // 保留最近3次的日志
    cleanOldLogs(3);
    //安装日志
    qInstallMessageHandler(LoggingHandler);

    splash.showMessage(QObject::tr("启动中..."), Qt::AlignLeft | Qt::AlignBottom, Qt::white);
    MainWindow w;
    QObject::connect(&w, &MainWindow::sigUpdatePlot, &splash, [&](const QString &log) {
        splash.showMessage(log, Qt::AlignLeft | Qt::AlignBottom, Qt::white);
    }/*, Qt::QueuedConnection */);

    QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();
    int x = (screenRect.width() - w.width()) / 2;
    int y = (screenRect.height() - w.height()) / 2;
    w.move(x, y);
    splash.finish(&w);// 关闭启动画面，显示主窗口
    w.show();

    return a.exec();
}
