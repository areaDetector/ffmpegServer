TOP=../..

include $(TOP)/configure/CONFIG

# -------------------------------
# Build an Diamond Support Module

CONFORM_CFLAGS_STRICT = 
CONFORM_CXXFLAGS_STRICT = 

LIBRARY_IOC += ffmpegServer

# xxxRecord.dbd will be installed into <top>/dbd
DBD += ffmpegServer.dbd

INC += ffmpegServer.h

USR_INCLUDES_WIN32 += -I$(TOP)/build

USR_CFLAGS += -std=c99
USR_CXXFLAGS += -U_GNU_SOURCE

SRC_DIRS += $(NULLHTTPD)/src
USR_CXXFLAGS_WIN32 += -DWIN32
USR_CFLAGS_WIN32 += -DWIN32

# The following are compiled and added to the support library
ffmpegServer_SRCS += ffmpegServer.cpp cgi.c files.c format.c http.c server.c win32.c ffmpegFile.cpp
ffmpegServer_LIBS += avdevice avformat avcodec avutil swscale

LIB_INSTALLS_WIN32 += $(wildcard $(TOP)/ffmpeg/mingw32/lib/*.lib)
BIN_INSTALLS_WIN32 += $(wildcard $(TOP)/ffmpeg/mingw32/bin/*.dll)

#ffmpegServer_SYS_LIBS_Linux += m dl

include $(TOP)/configure/RULES

