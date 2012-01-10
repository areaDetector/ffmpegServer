TOP = ..
include $(TOP)/configure/CONFIG

# config options for linux builds
CONFIGOPTIONS += --prefix=$(shell cd $(TOP)/vendor && pwd)/ffmpeg-$(T_A)
CONFIGOPTIONS += --disable-indev=v4l --disable-bzlib --disable-zlib --enable-shared --disable-static
CONFIGOPTIONS += --disable-ffmpeg --disable-ffplay --disable-ffprobe --disable-ffserver
CONFIGOPTIONS += --disable-indevs --disable-outdevs

# install libs
FFMPEG_LINUX_LIBS += libavcodec.so libavcodec.so.53 libavcodec.so.53.7.0 
FFMPEG_LINUX_LIBS += libavdevice.so libavdevice.so.53 libavdevice.so.53.1.1
FFMPEG_LINUX_LIBS += libavfilter.so libavfilter.so.2 libavfilter.so.2.23.0
FFMPEG_LINUX_LIBS += libavformat.so libavformat.so.53 libavformat.so.53.4.0
FFMPEG_LINUX_LIBS += libavutil.so libavutil.so.51 libavutil.so.51.9.1
FFMPEG_LINUX_LIBS += libswscale.so libswscale.so.2 libswscale.so.2.0.0

ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
	BIN_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win32-shared/bin/*.*)
	LIB_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win32-dev/lib/*.*)	
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
	BIN_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win64-shared/bin/*.*)
	LIB_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win64-dev/lib/*.*)	
else ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
	LIB_INSTALLS_Linux  += $(addprefix ../ffmpeg-linux-x86_64/lib/, $(FFMPEG_LINUX_LIBS))
	FFMPEGMAK = $(TOP)/vendor/ffmpeg-linux-x86_64/config.mak	
else ifeq (linux-x86, $(findstring linux-x86, $(T_A)))
	LIB_INSTALLS_Linux  += $(addprefix ../ffmpeg-linux-x86/lib/, $(FFMPEG_LINUX_LIBS))
	FFMPEGMAK = $(TOP)/vendor/ffmpeg-linux-x86/config.mak	
endif


install: $(FFMPEGMAK)

include $(TOP)/configure/RULES

$(TOP)/vendor/ffmpeg-%/config.mak: $(TOP)/vendor/yasm-%/yasm
	(mkdir -p $(TOP)/vendor/ffmpeg-$(*F); cd $(TOP)/vendor/ffmpeg-$(*F); export PATH=${PATH}:../yasm-$(*F); sh ../ffmpeg/configure $(CONFIGOPTIONS) && make && make install)

$(TOP)/vendor/yasm-%/yasm:
	(mkdir -p $(TOP)/vendor/yasm-$(*F); cd $(TOP)/vendor/yasm-$(*F); sh ../yasm/configure && make && touch $@)
