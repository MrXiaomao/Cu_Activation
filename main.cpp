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

#include <log4qt/log4qt.h>
#include <log4qt/logger.h>
#include <log4qt/layout.h>
#include <log4qt/patternlayout.h>
#include <log4qt/consoleappender.h>
#include <log4qt/dailyfileappender.h>
#include <log4qt/logmanager.h>
#include <log4qt/propertyconfigurator.h>
#include <log4qt/loggerrepository.h>
#include <log4qt/fileappender.h>

// QString QtMsgTypeToString(QtMsgType type)
// {
//     switch (type) {
//     case QtInfoMsg:
//         return "[信息]";     break;
//     case QtDebugMsg:
//         return "[调试]";    break;
//     case QtWarningMsg:
//         return "[警告]";  break;
//     case QtCriticalMsg:
//         return "[严重]"; break;
//     case QtFatalMsg:
//         return "[错误]";    break;
//     default:
//         return "Unknown";
//     }
// }

MainWindow *mw = nullptr;
QMutex mutexMsg;
QtMessageHandler system_default_message_handler = NULL;// 用来保存系统默认的输出接口
void AppMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &msg)
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

    if (mw && type != QtDebugMsg)
        emit mw->sigAppengMsg(msg + "\n", type);

    // // 确保logs目录存在
    // QDir dir(QDir::currentPath() + "/logs");
    // if (!dir.exists()) {
    //     dir.mkpath(".");
    // }

    // // 获取当前日期，并格式化为YYYY-MM-DD
    // QString currentDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    // // 创建日志文件路径，例如：logs/2023-10-23.log
    // QString logFilePath = QDir::currentPath() + "/logs/2Cu_Activation_" + currentDate + ".log";

    // // 打开文件以追加模式
    // QFile file(logFilePath);
    // if (file.open(QIODevice::Append | QIODevice::Text)) {
    //     QTextStream out(&file);
    //     QString strMessage = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz ")
    //                          + QtMsgTypeToString(type) + ": " + msg + "\n";
    //     out << strMessage;
    //     file.flush();
    //     file.close();
    // }

    //这里必须调用，否则消息被拦截，log4qt无法捕获系统日志
    if (system_default_message_handler){
        system_default_message_handler(type, context, msg);
    }
}

void logStartup()
{
    auto logger = Log4Qt::Logger::rootLogger();

    logger->info(QStringLiteral("################################################################"));
    logger->info(QStringLiteral("                           程序启动                               "));
    logger->info(QStringLiteral("################################################################"));
}

void shutdownRootLogger()
{
    auto logger = Log4Qt::Logger::rootLogger();
    logger->removeAllAppenders();
    logger->loggerRepository()->shutdown();
}

#ifdef WIN32
#  include "app_dmp.h"
#endif
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

    //注冊异常捕获函数
#ifdef WIN32
    ::SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
    qputenv("QT_QPA_PLATFORM", "windows:darkmode=2");
#endif

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

    // 启用新的日子记录类
    QString sConfFilename = "./log4qt.conf";
    QStringList args = QCoreApplication::arguments();
    if (args.contains("-m") && args.contains("offline"))
        sConfFilename = "./log4qt2.conf";

    if (QFileInfo::exists(sConfFilename)){
        Log4Qt::PropertyConfigurator::configure(sConfFilename);
    } else {
        Log4Qt::LogManager::setHandleQtMessages(true);
        Log4Qt::Logger *logger = Log4Qt::Logger::rootLogger();
        logger->setLevel(Log4Qt::Level::DEBUG_INT); //设置日志输出级别

        /****************PatternLayout配置日志的输出格式****************************/
        Log4Qt::PatternLayout *layout = new Log4Qt::PatternLayout();
        layout->setConversionPattern("%d{yyyy-MM-dd HH:mm:ss.zzz} [%p]: %m %n");
        layout->activateOptions();

        /***************************配置日志的输出位置***********/
        //输出到控制台
        Log4Qt::ConsoleAppender *appender = new Log4Qt::ConsoleAppender(layout, Log4Qt::ConsoleAppender::STDOUT_TARGET);
        appender->activateOptions();
        logger->addAppender(appender);

        //输出到文件(如果需要把离线处理单独保存日志文件，可以改这里)
        QStringList args = QCoreApplication::arguments();
        if (args.contains("-m") && args.contains("offline")) {
            Log4Qt::DailyFileAppender *dailiAppender = new Log4Qt::DailyFileAppender(layout, "logs/.log", "offline_yyyy-MM-dd");
            dailiAppender->setAppendFile(true);
            dailiAppender->activateOptions();
            logger->addAppender(dailiAppender);
        } else {
            Log4Qt::DailyFileAppender *dailiAppender = new Log4Qt::DailyFileAppender(layout, "logs/.log", "Cu_Activation_yyyy-MM-dd");
            dailiAppender->setAppendFile(true);
            dailiAppender->activateOptions();
            logger->addAppender(dailiAppender);
        }
    }

    // 确保logs目录存在
    QDir dir(QDir::currentPath() + "/logs");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    //logStartup();

    //安装日志，主要用户主界面刷新日志信息，日志写文件改为log4qt模块来实现了
    system_default_message_handler = qInstallMessageHandler(AppMessageHandler);

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

    int ret = a.exec();

    //运行运行到这里，此时主窗体析构函数还没触发，所以shutdownRootLogger需要在主窗体销毁以后再做处理
    QObject::connect(&w, &QObject::destroyed, []{
        shutdownRootLogger();
    });

    return ret;
}
