TOP=../..

include $(TOP)/configure/CONFIG

# -------------------------------
# Build an Diamond Support Module

CONFORM_CFLAGS_STRICT = 
CONFORM_CXXFLAGS_STRICT = 

LIBRARY_IOC += ffmpegServer

DBD += ffmpegServer.dbd
#INC += ffmpegServer.h

# ffmpeg must be built to c99 standard
USR_CFLAGS += -std=c99
USR_CXXFLAGS += -U_GNU_SOURCE

# Include the srcs from nullhttpd
SRC_DIRS += $(NULLHTTPD)/src

# For some reason the windows build needs stdint.h and inttypes.h
USR_INCLUDES_WIN32 += -I$(TOP)/vendor/ffmpeg-mingw32-shared/include
USR_INCLUDES_Linux += -I$(TOP)/vendor/ffmpeg/include
USR_CXXFLAGS_WIN32 += -DWIN32
USR_CFLAGS_WIN32 += -DWIN32

# The following are compiled and added to the support library
ffmpegServer_SRCS += ffmpegCommon.cpp
ffmpegServer_SRCS += ffmpegServer.cpp cgi.c files.c format.c http.c server.c win32.c 
ffmpegServer_SRCS += ffmpegFile.cpp
ffmpegServer_LIBS += avdevice avformat avcodec avutil swscale

include $(TOP)/configure/RULES

