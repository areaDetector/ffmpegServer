TOP = ..
include $(TOP)/configure/CONFIG

# install libs
FFMPEG_LINUX_LIBS = libavcodec.a libavdevice.a libavfilter.a libavformat.a libavutil.a libswscale.a
FFMPEG_LINUX_BINS = ffmpeg ffprobe ffserver
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
	BIN_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win32-shared/bin/*.*)
	LIB_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win32-dev/lib/*.*)	
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
	BIN_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win64-shared/bin/*.*)
	LIB_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win64-dev/lib/*.*)	
else ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
	BIN_INSTALLS_Linux  += $(addprefix ../ffmpeg-linux-x86_64/bin/, $(FFMPEG_LINUX_BINS))
	LIB_INSTALLS_Linux  += $(addprefix ../ffmpeg-linux-x86_64/lib/, $(FFMPEG_LINUX_LIBS))
	FFMPEGMAK = $(TOP)/vendor/ffmpeg-linux-x86_64/config.mak	
else ifeq (linux-x86, $(findstring linux-x86, $(T_A)))
	BIN_INSTALLS_Linux  += $(addprefix ../ffmpeg-linux-x86/bin/, $(FFMPEG_LINUX_BINS))
	LIB_INSTALLS_Linux  += $(addprefix ../ffmpeg-linux-x86/lib/, $(FFMPEG_LINUX_LIBS))
	FFMPEGMAK = $(TOP)/vendor/ffmpeg-linux-x86/config.mak	
endif

# config options for linux builds
CONFIGOPTIONS += --prefix=$(shell cd $(TOP)/vendor && pwd)/ffmpeg-$(T_A)
CONFIGOPTIONS += --disable-indev=v4l --disable-bzlib --disable-zlib

install: $(FFMPEGMAK)

include $(TOP)/configure/RULES

$(TOP)/vendor/ffmpeg-%/config.mak: $(TOP)/vendor/yasm-%/config.log
	(mkdir -p $(TOP)/vendor/ffmpeg-$(*F); cd $(TOP)/vendor/ffmpeg-$(*F); export PATH=${PATH}:../yasm-$(*F); sh ../ffmpeg/configure $(CONFIGOPTIONS) && make && make install)

$(TOP)/vendor/yasm-%/config.log:
	(mkdir -p $(TOP)/vendor/yasm-$(*F); cd $(TOP)/vendor/yasm-$(*F); sh ../yasm/configure && make)
