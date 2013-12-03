TOP = ..
include $(TOP)/configure/CONFIG

# config options for linux builds
CONFIGOPTIONS += --prefix=$(shell cd $(TOP)/vendor && pwd)/ffmpeg-$(T_A)
CONFIGOPTIONS += --enable-shared --enable-static --enable-debug --disable-stripping
CONFIGOPTIONS += --disable-ffmpeg --disable-ffplay --disable-ffprobe --disable-ffserver
CONFIGOPTIONS += --disable-indevs --disable-outdevs --disable-symver

# install libs
FFMPEG_LINUX_LIBS += libavcodec.a libavcodec.so libavcodec.so.54 libavcodec.so.54.86.100 
FFMPEG_LINUX_LIBS += libavdevice.a libavdevice.so libavdevice.so.54 libavdevice.so.54.3.102
FFMPEG_LINUX_LIBS += libavfilter.a libavfilter.so libavfilter.so.3 libavfilter.so.3.32.100
FFMPEG_LINUX_LIBS += libavformat.a libavformat.so libavformat.so.54 libavformat.so.54.59.106
FFMPEG_LINUX_LIBS += libavutil.a libavutil.so libavutil.so.52 libavutil.so.52.13.100
FFMPEG_LINUX_LIBS += libswresample.a libswresample.so libswresample.so.0 libswresample.so.0.17.102
FFMPEG_LINUX_LIBS += libswscale.a libswscale.so libswscale.so.2 libswscale.so.2.1.103

ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
	BIN_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win32-shared/bin/*.*)
	LIB_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win32-dev/lib/*.*)	
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
	BIN_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win64-shared/bin/*.dll)
	LIB_INSTALLS_WIN32  += $(wildcard ../ffmpeg-win64-dev/lib/*.lib)	
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
