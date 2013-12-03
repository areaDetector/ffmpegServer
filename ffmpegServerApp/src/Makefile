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
USR_CFLAGS_Linux += -std=c99

# For RHEL5 we need to undef _GNU_SOURCE
ifeq (R3.14.11, $(findstring R3.14.11, $(EPICS_BASE)))
USR_CXXFLAGS += -U_GNU_SOURCE
else ifeq (R3.14.12.1, $(findstring R3.14.12.1, $(EPICS_BASE)))
USR_CXXFLAGS += -U_GNU_SOURCE
endif

# Include the srcs from nullhttpd
SRC_DIRS += $(NULLHTTPD)/src

# For some reason the windows build needs stdint.h and inttypes.h
USR_INCLUDES_WIN32 += -I$(TOP)/vendor/msinttypes
# add the right include files
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
	USR_INCLUDES_WIN32 += -I$(TOP)/vendor/ffmpeg-win32-dev/include
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
	USR_INCLUDES_WIN32 += -I$(TOP)/vendor/ffmpeg-win64-dev/include
else ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
	USR_INCLUDES_Linux += -I$(TOP)/vendor/ffmpeg-linux-x86_64/include	
else ifeq (linux-x86, $(findstring linux-x86, $(T_A)))
	USR_INCLUDES_Linux += -I$(TOP)/vendor/ffmpeg-linux-x86/include
endif

USR_CXXFLAGS_WIN32 += -DWIN32
USR_CFLAGS_WIN32 += -DWIN32

# The following are compiled and added to the support library
ffmpegServer_SRCS += ffmpegCommon.cpp
ffmpegServer_SRCS += ffmpegServer.cpp cgi.c files.c format.c http.c server.c win32.c 
ffmpegServer_SRCS += ffmpegFile.cpp
ffmpegServer_LIBS += avdevice avformat avcodec avutil swscale

include $(TOP)/configure/RULES

