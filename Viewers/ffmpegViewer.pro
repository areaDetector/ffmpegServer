TEMPLATE = app
CONFIG += qt warn_on thread

HEADERS += colorMaps.h SSpinBox.h
SOURCES += ffmpegViewer.cpp SSpinBox.cpp

# uncomment these for Xv support
HEADERS += ffmpegWidget.h
SOURCES += ffmpegWidget.cpp
LIBS += -lXv

TARGET = ffmpegViewer
INCLUDEPATH += /dls_sw/prod/R3.14.11/support/ffmpegServer/1-7/vendor/ffmpeg-linux-x86/include
LIBPATH += /dls_sw/prod/R3.14.11/support/ffmpegServer/1-7/lib/linux-x86
QMAKE_RPATHDIR += /dls_sw/prod/R3.14.11/support/ffmpegServer/1-7/lib/linux-x86
LIBS += -lavdevice -lavformat -lavcodec -lavutil -lbz2 -lswscale 
QTDIR_build:REQUIRES="contains(QT_CONFIG, small-config)"
OBJECTS_DIR = O.linux-x86
MOC_DIR = O.linux-x86
DEFINES += __STDC_CONSTANT_MACROS
QMAKE_CLEAN += $$TARGET
target.path = bin
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS ffmpegViewer.pro
INSTALLS += target





