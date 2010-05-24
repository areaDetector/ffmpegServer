TEMPLATE = app
TARGET = ffmpegViewer
DESTDIR = .
LIBS += -lavdevice -lavformat -lavcodec -lavutil -lbz2 -lswscale
QT += opengl
CONFIG += thread release
SOURCES += main.cpp ffmpegViewer.cpp
HEADERS += ffmpegViewer.h

things.path = ../data
things.files = ffmpegViewer
things.depends = ffmpegViewer

INSTALLS += things
