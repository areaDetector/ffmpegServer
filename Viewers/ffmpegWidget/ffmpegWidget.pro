TEMPLATE = lib
CONFIG = staticlib
CONFIG += qt
HEADERS += colorMaps.h ffmpegWidget.h 
SOURCES += ffmpegWidget.cpp
QMAKE_CLEAN += libffmpegWidget.a
header_files.files = ffmpegWidget.h 
header_files.path = ../../include
target.path = ../../lib/linux-x86
INSTALLS += target header_files
QMAKE_CFLAGS = -m32
QMAKE_CXXFLAGS = -m32
QMAKE_LFLAGS = -m32

# ffmpeg stuff
INCLUDEPATH += ../../vendor/ffmpeg-linux-x86/include
LIBPATH += ../../lib/linux-x86
QMAKE_RPATHDIR += ../../lib/linux-x86
LIBS += -lavdevice -lavformat -lavcodec -lavutil -lbz2 -lswscale -lca
DEFINES += __STDC_CONSTANT_MACROS

