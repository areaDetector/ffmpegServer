TEMPLATE = app
TARGET = ffmpegViewer
DESTDIR = .
LIBS += -lavdevice -lavformat -lavcodec -lavutil -lbz2 -lswscale
QT += opengl
CONFIG += thread release
SOURCES += main.cpp videoWidget.cpp
HEADERS += videoWidget.h

things.path = ../data
things.files = ffmpegViewer
things.depends = ffmpegViewer

INSTALLS += things
