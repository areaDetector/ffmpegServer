TEMPLATE = app
CONFIG += qt warn_on thread
HEADERS += ffmpegViewer.h colorMaps.h SSpinBox.h
SOURCES += main.cpp ffmpegViewer.cpp SSpinBox.cpp

# uncomment these for Xv support
# HEADERS += ffmpegViewerXv.h
# SOURCES += ffmpegViewerXv.cpp
# LIBS += -lXv

QT += opengl
TARGET = ffmpegViewer
INCLUDEPATH += ../vendor/ffmpeg-linux-x86/include
LIBPATH += ../lib/linux-x86
QMAKE_RPATHDIR += ../lib/linux-x86
LIBS += -lavdevice -lavformat -lavcodec -lavutil -lbz2 -lswscale 
QTDIR_build:REQUIRES="contains(QT_CONFIG, small-config)"
OBJECTS_DIR = O.linux-x86
MOC_DIR = O.linux-x86
DEFINES += __STDC_CONSTANT_MACROS
QMAKE_CLEAN += $$TARGET
target.path = ../data
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS ffmpegViewer.pro
INSTALLS += target





