QT       += core gui widgets network

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
    aboutwidget.cpp \
    augmentedmatrix.cpp \
    commandhelper.cpp \
    coolingtimewidget.cpp \
    dataanalysiswidget.cpp \
    energycalibrationform.cpp \
    equipmentmanagementform.cpp \
    plotwidget.cpp \
    polynomialfit.cpp \
    qcustomplot.cpp \
    main.cpp \
    mainwindow.cpp \
    rollingtimewidget.cpp \
    spectrumModel.cpp \
    sysutils.cpp \
    waveformmodel.cpp \
    FPGASetting.cpp

HEADERS += \
    aboutwidget.h \
    augmentedmatrix.h \
    commandhelper.h \
    coolingtimewidget.h \
    dataanalysiswidget.h \
    energycalibrationform.h \
    equipmentmanagementform.h \
    plotwidget.h \
    polynomialfit.h \
    qcustomplot.h \
    mainwindow.h \
    rollingtimewidget.h \
    spectrumModel.h \
    sysutils.h \
    waveformmodel.h \
    FPGASetting.h

FORMS += \
    FPGASetting.ui \
    aboutwidget.ui \
    coolingtimewidget.ui \
    dataanalysiswidget.ui \
    energycalibrationform.ui \
    spectrumModel.ui \
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

# 软件图标
RC_ICONS = $$PWD/resource/logo.ico

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
