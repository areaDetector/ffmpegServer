TOP = ..
include $(TOP)/configure/CONFIG

# config options for linux builds
CONFIGOPTIONS += --prefix=$(shell cd $(TOP)/vendor && pwd)/ffmpeg-$(T_A)
CONFIGOPTIONS += --enable-shared --enable-static --enable-debug --disable-stripping
CONFIGOPTIONS += --disable-ffmpeg --disable-ffplay --disable-ffprobe --disable-ffserver
CONFIGOPTIONS += --disable-indevs --disable-outdevs --disable-symver

# Good idea to use these if you have to do any debugging...
# CONFIGOPTIONS += --disable-mmxext --disable-mmx --disable-optimizations

ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
	MACHINE = i386
	VENDORARCH = win32
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))	
	MACHINE = X64
	VENDORARCH = win64
else ifeq (linux-x86, $(findstring linux-x86, $(T_A)))
	FFMPEGMAK = $(TOP)/vendor/ffmpeg-$(T_A)/config.mak	
endif

# For windows, install the dlls and libs
BIN_INSTALLS_WIN32  += $(wildcard ../ffmpeg-$(VENDORARCH)-shared/bin/*.dll)
LIB_INSTALLS_WIN32  += avdevice.lib avformat.lib avcodec.lib avutil.lib swscale.lib

install: $(FFMPEGMAK)

include $(TOP)/configure/RULES

$(TOP)/vendor/ffmpeg-%/config.mak: $(TOP)/vendor/yasm-%/yasm
	mkdir -p $(TOP)/vendor/ffmpeg-$(*F)
	cd $(TOP)/vendor/ffmpeg-$(*F) && export PATH=${PATH}:../yasm-$(*F) && sh ../ffmpeg/configure $(CONFIGOPTIONS) && make && make install

$(TOP)/vendor/yasm-%/yasm:
	mkdir -p $(TOP)/vendor/yasm-$(*F)
	cd $(TOP)/vendor/yasm-$(*F) && sh ../yasm/configure && make
	touch $@
	
# unfortunately the libs are not compatible with the opt/ref flag and pre-2012
# MSVC compilers, so we'll have to make our own...
# on windows, we can make libs out of defs by using lib.exe as listed on
# http://ffmpeg.org/platform.html#Linking-to-FFmpeg-with-Microsoft-Visual-C_002b_002b
%.lib:
	lib /machine:$(MACHINE) /def:$(wildcard ../ffmpeg-$(VENDORARCH)-dev/lib/$(*F)-*.def) /out:$@
	
