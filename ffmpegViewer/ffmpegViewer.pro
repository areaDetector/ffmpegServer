TEMPLATE = app
TARGET =
DEPENDPATH += .
INCLUDEPATH += .

# Input
SOURCES += main.cpp videoWidget.cpp
HEADERS += videoWidget.h

# ffmpeg libs for ffmpegViewer widget
INCLUDEPATH += ../../include
LIBPATH += ../../lib/linux-x86
LIBS += -lavdevice -lavformat -lavcodec -lavutil -lbz2 -lswscale
QT += opengl
CONFIG += thread release

# install
ffmpegViewer.path = ../data
ffmpegViewer.files = ffmpegViewer
INSTALLS += ffmpegViewer
