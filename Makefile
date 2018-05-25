#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) configure
DIRS := $(DIRS) vendor
DIRS := $(DIRS) ffmpegServerApp
ffmpegServerApp_DEPEND_DIRS += vendor
include $(TOP)/configure/RULES_TOP
