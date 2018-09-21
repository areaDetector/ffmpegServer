TOP=../..

include $(TOP)/configure/CONFIG

# -------------------------------
# Build an Diamond Support Module

CONFORM_CFLAGS_STRICT = 
CONFORM_CXXFLAGS_STRICT = 

LIBRARY_IOC += ffmpegServer

DBD += ffmpegServer.dbd
INC += ffmpegServer.h

# ffmpeg must be built to c99 standard
USR_CFLAGS_Linux += -std=c99

# For RHEL5 we need to undef _GNU_SOURCE
ifeq (R3.14.11, $(findstring R3.14.11, $(EPICS_BASE)))
USR_CXXFLAGS += -U_GNU_SOURCE
else ifeq (R3.14.12.1, $(findstring R3.14.12.1, $(EPICS_BASE)))
USR_CXXFLAGS += -U_GNU_SOURCE
endif

# For some reason the windows build needs stdint.h and inttypes.h
USR_INCLUDES_WIN32 += -I$(TOP)/vendor/msinttypes

# And we need to switch nullhttpd to win32 mode
USR_CXXFLAGS_WIN32 += -DWIN32
USR_CFLAGS_WIN32 += -DWIN32

# point to the right vendor architecture
VENDORARCH=$(T_A)
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
	VENDORARCH=win32-dev
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
	VENDORARCH=win64-dev
endif

# Include the ffmpeg header files
USR_INCLUDES += -I$(TOP)/vendor/ffmpeg-$(VENDORARCH)/include
		
# build the ffmpeg libs (or stubs on windows) into the binaries
LIB_LIBS += avdevice avformat avcodec swresample swscale avutil
LIB_LIBS += NDPlugin
LIB_SYS_LIBS_WIN32 += user32
		
# The following are compiled and added to the support library
ffmpegServer_SRCS += ffmpegCommon.cpp
ffmpegServer_SRCS += ffmpegServer.cpp nullhttpd_server.c nullhttpd_http.c nullhttpd_format.c nullhttpd_win32.c
ffmpegServer_SRCS += ffmpegFile.cpp

include $(ADCORE)/ADApp/commonLibraryMakefile

include $(TOP)/configure/RULES

	
