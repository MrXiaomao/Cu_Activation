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
    cachedirconfigwidget.cpp \
    coincidenceanalyzer.cpp \
    controlhelper.cpp \
    controlwidget.cpp \
    function.cpp \
    aboutwidget.cpp \
    augmentedmatrix.cpp \
    commandhelper.cpp \
    coolingtimewidget.cpp \
    dataanalysiswidget.cpp \
    energycalibrationform.cpp \
    equipmentmanagementform.cpp \
    linearfit.cpp \
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
    FPGASetting.cpp

HEADERS += \
    cachedirconfigwidget.h \
    coincidenceanalyzer.h \
    controlhelper.h \
    controlwidget.h \
    function.h \
    aboutwidget.h \
    augmentedmatrix.h \
    commandhelper.h \
    coolingtimewidget.h \
    dataanalysiswidget.h \
    energycalibrationform.h \
    equipmentmanagementform.h \
    linearfit.h \
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
    FPGASetting.h

FORMS += \
    FPGASetting.ui \
    aboutwidget.ui \
    cachedirconfigwidget.ui \
    controlwidget.ui \
    coolingtimewidget.ui \
    dataanalysiswidget.ui \
    energycalibrationform.ui \
    spectrumModel.ui \
    splashwidget.ui \
    waveformmodel.ui \
    equipmentmanagementform.ui \
    mainwindow.ui

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

# 定义输出目录变量
OUTPUT_DIR_BASE = $$PWD/../../build

#message($${TARGET}":" "$$QT_ARCH" "$$QMAKE_CXX")

# 根据编译器设置输出目录
win32:msvc {
    # MSVC 编译器
    QMAKE_CXXFLAGS += -utf-8
    QMAKE_CXXFLAGS += /MP

    OUTPUT_DIR = $$OUTPUT_DIR_BASE/msvc
    contains(QT_ARCH, x86_64) {
        # MSVC x64
        OUTPUT_DIR = $$OUTPUT_DIR"/x64"
    } else {
        # MSVC x86
        OUTPUT_DIR = $$OUTPUT_DIR"/x86"
    }
}

win32:mingw {
    # MinGW 编译器
    OUTPUT_DIR = $$OUTPUT_DIR_BASE/mingw
    contains(QT_ARCH, x86_64) {
        # MinGW x64
        OUTPUT_DIR = $$OUTPUT_DIR/x64
    } else {
        # MinGW x86
        OUTPUT_DIR = $$OUTPUT_DIR/x86
    }
}

unix{
    contains( QMAKE_CXX, "g++" ) :{
        OUTPUT_DIR = $$OUTPUT_DIR_BASE"/g++"
    } else : contains( QMAKE_CXX, "aarch64-linux-gnu-g++" ) :{
        OUTPUT_DIR = $$OUTPUT_DIR_BASE"/g++"
    } else : contains( QMAKE_CXX, "gcc" ) :{
        OUTPUT_DIR = $$OUTPUT_DIR_BASE"/gcc"
    } else :  contains( QMAKE_CC, "aarch64-linux-gnu-gcc" ) :{
        OUTPUT_DIR = $$OUTPUT_DIR_BASE"/gcc"
    } else {
        OUTPUT_DIR = $$OUTPUT_DIR_BASE"/gcc"
    }
}

win32:CONFIG(release, debug|release): OUTPUT_DIR = $$OUTPUT_DIR/release
else:win32:CONFIG(debug, debug|release): OUTPUT_DIR = $$OUTPUT_DIR/debug

# 设置输出目录
#message(Qt Version = $$[QT_VERSION])
#QT_VERSION = $$split(QT_VERSION, ".")
#QT_VER_MAJ = $$member(QT_VERSION, 0)
#QT_VER_MIN = $$member(QT_VERSION, 1)

greaterThan(QT_MAJOR_VERSION, 5){
    DESTDIR = $$OUTPUT_DIR/qt6/
}
else{
    DESTDIR = $$OUTPUT_DIR/qt5/
}
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

#message(GIT_BRANCH":  ""$$GIT_BRANCH")
#message(GIT_TIME":  ""$$GIT_TIME")
#message(APP_VERSION":  ""$$APP_VERSION")

contains(QT_ARCH, x86_64) {
    win32: LIBS += -L$$PWD/3rdParty/ftcoreimc_win_v1.3.0.9n/lib/x64/ -lftcoreimc
    DEPENDPATH += $$PWD/3rdParty/ftcoreimc_win_v1.3.0.9n/lib/x64/
} else {
    win32: LIBS += -L$$PWD/3rdParty/ftcoreimc_win_v1.3.0.9n/lib/x86/ -lftcoreimc
    DEPENDPATH += $$PWD/3rdParty/ftcoreimc_win_v1.3.0.9n/lib/x86/
}

INCLUDEPATH += $$PWD/3rdParty/ftcoreimc_win_v1.3.0.9n/inc

#win32: LIBS += -L$$PWD/dwarfstack-2.2/lib/ -ldwarfstack

#INCLUDEPATH += $$PWD/dwarfstack-2.2/include
#DEPENDPATH += $$PWD/dwarfstack-2.2/include

#win32:!win32-g++: PRE_TARGETDEPS += $$PWD/dwarfstack-2.2/lib/dwarfstack.lib
#else:win32-g++: PRE_TARGETDEPS += $$PWD/dwarfstack-2.2/lib/libdwarfstack.a
