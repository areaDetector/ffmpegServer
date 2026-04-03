#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) configure
DIRS := $(DIRS) ffmpegServerApp
ifeq (FFMPEG_EXTERNAL, NO)
DIRS := $(DIRS) vendor
ffmpegServerApp_DEPEND_DIRS += vendor
endif
include $(TOP)/configure/RULES_TOP
