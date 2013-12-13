TARGET = ffmpegViewer
HEADERS += SSpinBox.h caValueMonitor.h
SOURCES += ffmpegViewer.cpp SSpinBox.cpp caValueMonitor.cpp
FORMS += ffmpegViewer.ui  
target.path = ../../data
INSTALLS += target
INCLUDEPATH += ../ffmpegWidget
LIBS += -L../ffmpegWidget -lffmpegWidget
QMAKE_CLEAN += $$TARGET
QMAKE_CFLAGS = -m32
QMAKE_CXXFLAGS = -m32
QMAKE_LFLAGS = -m32

# epics base stuff
INCLUDEPATH += $$(EPICS_BASE)/include
INCLUDEPATH += $$(EPICS_BASE)/include/os/Linux
LIBPATH += $$(EPICS_BASE)/lib/linux-x86
QMAKE_RPATHDIR += $$(EPICS_BASE)/lib/linux-x86
LIBS += -lca

# ffmpeg stuff
INCLUDEPATH += ../../vendor/ffmpeg-linux-x86/include
LIBPATH += ../../vendor/ffmpeg-linux-x86/lib
QMAKE_RPATHDIR += ../../vendor/ffmpeg-linux-x86/lib
LIBS += -lavdevice -lavformat -lavcodec -lavutil -lbz2 -lswscale
DEFINES += __STDC_CONSTANT_MACROS

# xvideo stuff
LIBS += -lXv
