TEMPLATE = app
CONFIG += qt warn_on thread
HEADERS += ffmpegViewer.h colorMaps.h
SOURCES += main.cpp ffmpegViewer.cpp
QT += opengl
TARGET = ffmpegViewer
INCLUDEPATH += ../vendor/ffmpeg/include
LIBPATH += ../lib/linux-x86
LIBS += -lavdevice -lavformat -lavcodec -lavutil -lbz2 -lswscale
QTDIR_build:REQUIRES="contains(QT_CONFIG, small-config)"
OBJECTS_DIR = O.linux-x86
MOC_DIR = O.linux-x86
DEFINES += __STDC_CONSTANT_MACROS
QMAKE_CLEAN += $$TARGET
target.path = ../data
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS ffmpegViewer.pro
INSTALLS += target





