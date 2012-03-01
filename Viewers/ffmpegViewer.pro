# sources
HEADERS += colorMaps.h SSpinBox.h ffmpegWidget.h caValueMonitor.h
SOURCES += ffmpegViewer.cpp SSpinBox.cpp ffmpegWidget.cpp caValueMonitor.cpp
FORMS += ffmpegViewer.ui

# xvideo stuff
LIBS += -lXv

# ffmpeg stuff
INCLUDEPATH += ../vendor/ffmpeg-linux-x86/include
LIBPATH += ../lib/linux-x86
QMAKE_RPATHDIR += ../lib/linux-x86
LIBS += -lavdevice -lavformat -lavcodec -lavutil -lbz2 -lswscale -lca

# epics base stuff
INCLUDEPATH += $$(EPICS_BASE)/include
INCLUDEPATH += $$(EPICS_BASE)/include/os/Linux
LIBPATH += $$(EPICS_BASE)/lib/linux-x86
QMAKE_RPATHDIR += $$(EPICS_BASE)/lib/linux-x86

# other stuff
TEMPLATE = app
CONFIG += qt warn_on thread debug
TARGET = ffmpegViewer
QTDIR_build:REQUIRES="contains(QT_CONFIG, small-config)"
DEFINES += __STDC_CONSTANT_MACROS
QMAKE_CLEAN += $$TARGET
target.path = ../data
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS ffmpegViewer.pro
INSTALLS += target





