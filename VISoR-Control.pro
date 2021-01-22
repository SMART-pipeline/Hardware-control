QT += core serialport
QT -= gui

CONFIG += c++11

TARGET = VISoR-Control
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$(OPENCV_DIR)\include
LIBS += -L$$(OPENCV_DIR)\x64\vc14\lib -lopencv_world340

INCLUDEPATH += $$(LIB_PATH)\Thorlabs\Kinesis
LIBS += -L$$(LIB_PATH)\Thorlabs\Kinesis -lThorlabs.MotionControl.KCube.BrushlessMotor -lThorlabs.MotionControl.IntegratedStepperMotors

INCLUDEPATH += $$(LIB_PATH)/NI/C/include
LIBS += -L$$(LIB_PATH)/NI/C/lib64/msvc -lNIDAQmx -lnisyscfg

INCLUDEPATH += $$(LIB_PATH)\dcamsdk4\inc
LIBS += -L$$(LIB_PATH)\dcamsdk4\lib\win64 -ldcamapi

DEFINES += QT_DEPRECATED_WARNINGS
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    dcam_common.cpp \
    kinesisstage.cpp \
    masterthread.cpp \
    niworker.cpp \
    obisctrl.cpp \
    zaberstagectrl.cpp \
    hardware.cpp \
    imagewriter.cpp \
    lzwcodec.cpp

HEADERS += \
    dcam_common.h \
    kinesisstage.h \
    masterthread.h \
    niworker.h \
    obisctrl.h \
    zaberstagectrl.h \
    hardware.h \
    imagewriter.h \
    lzwcodec.h
