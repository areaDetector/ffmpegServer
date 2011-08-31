TOP = ..
include $(TOP)/configure/CONFIG
ifeq ($(EPICS_HOST_ARCH),linux-x86)
	CONFIGOPTIONS += --prefix=$(shell cd $(TOP)/vendor/ffmpeg && pwd)
	CONFIGOPTIONS += --disable-indev=v4l --disable-bzlib --disable-zlib
	FFMPEGMAK = $(TOP)/vendor/ffmpeg/config.mak   
endif

BIN_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win32-shared/bin/*.*)
LIB_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win32-dev/lib/*.*)
BIN_INSTALLS_Linux  += $(wildcard ../ffmpeg/bin/*)
LIB_INSTALLS_Linux  += $(wildcard ../ffmpeg/lib/*.a)

install: $(FFMPEGMAK)

include $(TOP)/configure/RULES

$(TOP)/vendor/ffmpeg/config.mak: $(TOP)/vendor/yasm/config.status
	(cd $(TOP)/vendor/ffmpeg; export PATH=${PATH}:../yasm; sh configure $(CONFIGOPTIONS) && make && make install)

$(TOP)/vendor/yasm/config.status:
	(cd $(TOP)/vendor/yasm; sh configure && make)
