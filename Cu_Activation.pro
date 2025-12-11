QT       += core gui widgets network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    QImageProgressBar.cpp \
    cachedirconfigwidget.cpp \
    coincidenceanalyzer.cpp \
    controlhelper.cpp \
    controlwidget.cpp \
    function.cpp \
    aboutwidget.cpp \
    augmentedmatrix.cpp \
    commandhelper.cpp \
    coolingtimewidget.cpp \
    energycalibrationform.cpp \
    equipmentmanagementform.cpp \
    gaussFit.cpp \
    globalsettings.cpp \
    linearfit.cpp \
    offlinedataanalysiswidget.cpp \
    plotwidget.cpp \
    polynomialfit.cpp \
    qcustomplot.cpp \
    main.cpp \
    mainwindow.cpp \
    qlitethread.cpp \
    spectrumModel.cpp \
    splashwidget.cpp \
    sysutils.cpp \
    waveformmodel.cpp \
    FPGASetting.cpp \
    yieldcalibration.cpp

HEADERS += \
    QImageProgressBar.h \
    cachedirconfigwidget.h \
    coincidenceanalyzer.h \
    controlhelper.h \
    controlwidget.h \
    function.h \
    aboutwidget.h \
    augmentedmatrix.h \
    commandhelper.h \
    coolingtimewidget.h \
    energycalibrationform.h \
    equipmentmanagementform.h \
    gaussFit.h \
    globalsettings.h \
    linearfit.h \
    offlinedataanalysiswidget.h \
    pch.h \
    plotwidget.h \
    polynomialfit.h \
    qcustomplot.h \
    mainwindow.h \
    qlitethread.h \
    spectrumModel.h \
    splashwidget.h \
    sysutils.h \
    waveformmodel.h \
    FPGASetting.h \
    yieldcalibration.h

FORMS += \
    FPGASetting.ui \
    aboutwidget.ui \
    cachedirconfigwidget.ui \
    controlwidget.ui \
    coolingtimewidget.ui \
    energycalibrationform.ui \
    offlinedataanalysiswidget.ui \
    spectrumModel.ui \
    splashwidget.ui \
    waveformmodel.ui \
    equipmentmanagementform.ui \
    mainwindow.ui \
    yieldcalibration.ui

TRANSLATIONS += \
    Cu_Activation_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# QCustomPlot所需
QT       += printsupport

# 启用OpenGL硬件加速
#DEFINES  += QCUSTOMPLOT_USE_OPENGL
#QT       += opengl
#LIBS     += -lopengl32 -lglu32

# 图片资源文件
RESOURCES += \
    resource.qrc

# 多语言资源文件，让右键菜单显示中文
RESOURCES += \
    qm.qrc

##############################################################################################################
# 软件图标
RC_ICONS = $$PWD/resource/logo.ico

# 指定要使用的预编译头文件
#CONFIG += precompile_header
#PRECOMPILED_HEADER += pch.h
#message($${TARGET}":" "$$QT_ARCH" "$$QMAKE_CXX")

# 设置输出目录
#message(Qt Version = $$[QT_VERSION])
# QT_VERSION = $$split(QT_VERSION, ".")
# QT_VER_MAJ = $$member(QT_VERSION, 0)
# QT_VER_MIN = $$member(QT_VERSION, 1)

#DESTDIR = $$PWD/../build
DESTDIR = $$PWD/../build_Cu
contains(QT_ARCH, x86_64) {
    # x64
    DESTDIR = $$DESTDIR/x64
} else {
    # x86
    DESTDIR = $$DESTDIR/x86
}

DESTDIR = $$DESTDIR/qt$$QT_VERSION/
message(DESTDIR = $$DESTDIR)

TARGET = Cu_Activation

# 避免创建空的debug和release目录
CONFIG -= debug_and_release

#指定编译产生的文件分门别类放到对应目录
MOC_DIR     = temp/moc
RCC_DIR     = temp/rcc
UI_DIR      = temp/ui
OBJECTS_DIR = temp/obj

#把所有警告都关掉眼不见为净
CONFIG += warn_off

#开启大资源支持
CONFIG += resources_big

#############################################################################################################
exists (./.git) {
    GIT_BRANCH   = $$system(git rev-parse --abbrev-ref HEAD)
    GIT_TIME     = $$system(git show --oneline --format=\"%ci%H\" -s HEAD)
    APP_VERSION = "Git: $${GIT_BRANCH}: $${GIT_TIME}"
} else {
    GIT_BRANCH      = None
    GIT_TIME        = None
    APP_VERSION     = None
}

DEFINES += GIT_BRANCH=\"\\\"$$GIT_BRANCH\\\"\"
DEFINES += GIT_TIME=\"\\\"$$GIT_TIME\\\"\"
DEFINES += APP_VERSION=\"\\\"$$APP_VERSION\\\"\"

message(GIT_BRANCH":  ""$$GIT_BRANCH")
message(GIT_TIME":  ""$$GIT_TIME")
message(APP_VERSION":  ""$$APP_VERSION")

contains(QT_ARCH, x86_64) {
    win32: LIBS += -L$$PWD/3rdParty/ftcoreimc_win_v1.4.0.0/lib/x64/ -lftcoreimc
    DEPENDPATH += $$PWD/3rdParty/ftcoreimc_win_v1.4.0.0/lib/x64/
} else {
    win32: LIBS += -L$$PWD/3rdParty/ftcoreimc_win_v1.4.0.0/lib/x86/ -lftcoreimc
    DEPENDPATH += $$PWD/3rdParty/ftcoreimc_win_v1.4.0.0/lib/x86/
}

INCLUDEPATH += $$PWD/3rdParty/ftcoreimc_win_v1.4.0.0/inc

# 指定 MSYS2 的 GSL 路径（64位示例）
win32 {
    # GSL 头文件路径
    INCLUDEPATH += "$$PWD/3rdParty/gsl/include"
    
    # GSL 库路径
    LIBS += -L$$PWD/3rdParty/gsl/lib -lgsl -lgslcblas
    # LIBS += -L"$$PWD/3rdParty/gsl/lib" -lgsl -lgslcblas
    
    # 如果使用动态链接库（DLL），确保运行时能找到它们
    # 方法1：将 DLL 复制到程序目录
    # 方法2：将 MSYS2 的 bin 目录加入 PATH
    # (例如 C:\msys64\mingw64\bin)
}

windows {
    # MinGW
    *-g++* {
        QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic
    }
    # MSVC
    *-msvc* {
        QMAKE_CXXFLAGS += /utf-8
    }
}

LOG4QTSRCPATH = $$PWD/3rdParty/log4qt/Include
INCLUDEPATH += -L $$LOG4QTSRCPATH \
                $$LOG4QTSRCPATH/helpers \
                 $$LOG4QTSRCPATH/spi \
                 $$LOG4QTSRCPATH/varia

DEPENDPATH  +=  $$LOG4QTSRCPATH \
            $$LOG4QTSRCPATH/helpers \
            $$LOG4QTSRCPATH/spi \
            $$LOG4QTSRCPATH/varia
include($$PWD/3rdParty/log4qt/Include/log4qt/log4qt.pri)

# win32:CONFIG(release, debug|release): LIBS += -L$$PWD/3rdParty/log4qt/lib -llog4qt
# else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/3rdParty/log4qt/lib -llog4qt

# INCLUDEPATH += $$PWD/3rdParty/log4qt/Include
# DEPENDPATH += $$PWD/3rdParty/log4qt/include
