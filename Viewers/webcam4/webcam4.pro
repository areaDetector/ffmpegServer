TARGET = webcam4
SOURCES += webcam4.cpp
FORMS += webcam4.ui  
target.path = ../../data
INSTALLS += target
INCLUDEPATH += ../ffmpegWidget
LIBS += -L../ffmpegWidget -lffmpegWidget
QMAKE_CLEAN += $$TARGET
QMAKE_CFLAGS = -m32
QMAKE_CXXFLAGS = -m32
QMAKE_LFLAGS = -m32

# ffmpeg stuff
INCLUDEPATH += ../../vendor/ffmpeg-linux-x86/include
LIBPATH += ../../lib/linux-x86
QMAKE_RPATHDIR += ../../lib/linux-x86
LIBS += -lavdevice -lavformat -lavcodec -lavutil -lbz2 -lswscale
DEFINES += __STDC_CONSTANT_MACROS

# xvideo stuff
LIBS += -lXv

