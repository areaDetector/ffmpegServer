TOP = ..
include $(TOP)/configure/CONFIG

# linux systems build all examples that don't end with win32
ifeq ($(EPICS_HOST_ARCH),linux-x86)
	DIRS := $(DIRS) $(filter-out $(wildcard Makefile*) $(wildcard *_win32), $(wildcard *))
# windows just build the _win32 example	
else ifeq ($(EPICS_HOST_ARCH),win32-x86)
	DIRS := $(DIRS) $(wildcard *_win32)
endif

include $(TOP)/configure/RULES_DIRS
