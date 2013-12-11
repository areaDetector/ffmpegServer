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

# And we need to switch nullhttpd to win32 mode
USR_CXXFLAGS_WIN32 += -DWIN32
USR_CFLAGS_WIN32 += -DWIN32

# These are the libs we get from ffmpeg
FFMPEG_LIBS = avdevice avformat avcodec avutil swscale	

# point to the right vendor architecture
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
	VENDORARCH=win32-dev
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
	VENDORARCH=win64-dev
else ifeq (linux-x86, $(findstring linux-x86, $(T_A)))
	# linux-x86 and linux-x86_64
	VENDORARCH=$(T_A)
	VENDORLIB = $(TOP)/vendor/ffmpeg-$(VENDORARCH)/lib
	avdevice_DIR = $(VENDORLIB)
	avformat_DIR = $(VENDORLIB)
	avcodec_DIR = $(VENDORLIB)
	avutil_DIR = $(VENDORLIB)
	swscale_DIR = $(VENDORLIB)	
endif

# Include the ffmpeg header files
USR_INCLUDES += -I$(TOP)/vendor/ffmpeg-$(VENDORARCH)/include
		
# build the ffmpeg libs (or stubs on windows) into the binaries
ffmpegServer_LIBS += $(FFMPEG_LIBS)		
		
# The following are compiled and added to the support library
ffmpegServer_SRCS += ffmpegCommon.cpp
ffmpegServer_SRCS += ffmpegServer.cpp server.c http.c format.c win32.c 
ffmpegServer_SRCS += ffmpegFile.cpp

include $(TOP)/configure/RULES

	
