#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QScreen>

int main(int argc, char *argv[])
{
    QApplication::setStyle(QStyleFactory::create("fusion"));//WindowsVista fusion windows
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling); // 启用高DPI缩放支持
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps); // 使用高DPI位图
    QApplication a(argc, argv);

    MainWindow w;
    QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();
    int x = (screenRect.width() - w.width()) / 2;
    int y = (screenRect.height() - w.height()) / 2;
    w.move(x, y);

    w.show();

    return a.exec();
}
