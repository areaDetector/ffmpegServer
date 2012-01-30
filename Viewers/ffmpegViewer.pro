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
INCLUDEPATH += /dls_sw/epics/R3.14.11/base/include
INCLUDEPATH += /dls_sw/epics/R3.14.11/base/include/os/Linux
LIBPATH += /dls_sw/epics/R3.14.11/base/lib/linux-x86
QMAKE_RPATHDIR += /dls_sw/epics/R3.14.11/base/lib/linux-x86

# other stuff
TEMPLATE = app
CONFIG += qt warn_on thread debug
TARGET = ffmpegViewer
QTDIR_build:REQUIRES="contains(QT_CONFIG, small-config)"
OBJECTS_DIR = O.linux-x86
MOC_DIR = O.linux-x86
DEFINES += __STDC_CONSTANT_MACROS
QMAKE_CLEAN += $$TARGET
target.path = ../data
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS ffmpegViewer.pro
INSTALLS += target





