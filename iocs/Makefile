TOP = ..
include $(TOP)/configure/CONFIG

ifeq ($(EPICS_HOST_ARCH), linux-x86)
	DIRS := $(wildcard example)
else ifeq ($(EPICS_HOST_ARCH), linux-x86_64)
	DIRS := $(wildcard example)
else ifeq ($(EPICS_HOST_ARCH), win32-x86)
	DIRS := $(wildcard example_win32)
else ifeq ($(EPICS_HOST_ARCH), windows-x64)
	DIRS := $(wildcard example_win32)	
endif

include $(TOP)/configure/RULES_DIRS
