#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QScreen>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QScreen>
#include <QSplashScreen>
#include <QMutex>
#include <QTranslator>
#include <iostream>

QString QtMsgTypeToString(QtMsgType type)
{
    switch (type) {
    case QtInfoMsg:
        return "[信息]";     break;
    case QtDebugMsg:
        return "[调试]";    break;
    case QtWarningMsg:
        return "[警告]";  break;
    case QtCriticalMsg:
        return "[严重]"; break;
    case QtFatalMsg:
        return "[错误]";    break;
    default:
        return "Unknown";
    }
}

MainWindow *mw = nullptr;
QMutex mutexMsg;
void AppMessageHandler(QtMsgType type, const QMessageLogContext& /*context*/, const QString &msg)
{
    //Release 版本默认不包含context这些信息:文件名、函数名、行数，需要在.pro项目文件加入以下代码，加入后最好重新构建项目使之生效：
    //DEFINES += QT_MESSAGELOGCONTEXT

    //在.pro文件定义以下的宏，可以屏蔽相应的日志输出
    //DEFINES += QT_NO_WARNING_OUTPUT
    //DEFINES += QT_NO_DEBUG_OUTPUT
    //DEFINES += QT_NO_INFO_OUTPUT
    //文件名、函数名、行数
    // strMsg += QString("Function: %1  File: %2  Line: %3 ").arg(context.function).arg(context.file).arg(context.line);

    // 加锁
    QMutexLocker locker(&mutexMsg);
    if (type == QtWarningMsg)
        return;

    if (type == QtDebugMsg){
        std::cout << "[Debug] " << msg.toLocal8Bit().toStdString() << std::endl;;
        // return;
    }

    // 确保logs目录存在
    QDir dir(QDir::currentPath() + "/logs");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 获取当前日期，并格式化为YYYY-MM-DD
    QString currentDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    // 创建日志文件路径，例如：logs/2023-10-23.log
    QString logFilePath = QDir::currentPath() + "/logs/Cu_Activation_" + currentDate + ".log";

    // 打开文件以追加模式
    QFile file(logFilePath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        QString strMessage = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz ")
                          + QtMsgTypeToString(type) + ": " + msg + "\n";
        out << strMessage;
        file.flush();
        file.close();

        if (mw && type != QtDebugMsg)
            emit mw->sigAppengMsg(msg + "\n", type);
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
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling); // 禁用高DPI缩放支持
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps); // 使用高DPI位图
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication a(argc, argv);
    if (qEnvironmentVariableIsEmpty("QT_FONT_DPI"))
    {
        qputenv("QT_FONT_DPI", "96");
        qputenv("QT_SCALE_FACTOR", "1.0");
    }

    qDebug() << APP_VERSION;
    qDebug() << GIT_BRANCH;

    QTranslator translator;
    if (translator.load(QLocale(), QLatin1String("Cu_Activation"), QLatin1String("_zh_CN.qm"), QLatin1String(":/i18n"))){
        a.installTranslator(&translator);
    }

    QTranslator qtBaseTranslator;
    if (qtBaseTranslator.load(":/qm/qt_zh_CN.qm")){
        a.installTranslator(&qtBaseTranslator);
    }

    QTranslator qtWidgetTranslator;
    if (qtWidgetTranslator.load(":/qm/widgets.qm")){
        a.installTranslator(&qtWidgetTranslator);
    }

    ///
    QSplashScreen splash;
    splash.setPixmap(QPixmap(":/resource/splash.png"));
    splash.show();

    // 保留最近3次的日志
    // cleanOldLogs(3);
    //安装日志
    qInstallMessageHandler(AppMessageHandler);

    splash.showMessage(QObject::tr("启动中..."), Qt::AlignLeft | Qt::AlignBottom, Qt::white);
    MainWindow w;
    mw = &w;

    qInfo().noquote() << QObject::tr("系统启动");
    QObject::connect(&w, &MainWindow::sigRefreshBoostMsg, &splash, [&](const QString &msg) {
        splash.showMessage(msg, Qt::AlignLeft | Qt::AlignBottom, Qt::white);
    }/*, Qt::QueuedConnection */);

    QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();
    int x = (screenRect.width() - w.width()) / 2;
    int y = (screenRect.height() - w.height()) / 2;
    w.move(x, y);
    splash.finish(&w);// 关闭启动画面，显示主窗口
    w.show();

    return a.exec();
}
