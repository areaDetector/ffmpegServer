#Makefile at top of application tree
TOP = .

include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), vendor)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *App))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *app))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocBoot))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocboot))
ifeq ($(EPICS_HOST_ARCH),linux-x86)
    DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard etc))
    # Comment out the following line to disable building of example iocs
    DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocs))
    # This builds the QT viewer
    DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard Viewers))
endif

install: $(TOP)/Viewers/Makefile

include $(TOP)/configure/RULES_TOP

$(TOP)/Viewers/Makefile: $(TOP)/Viewers/ffmpegViewer.pro
	/dls_sw/prod/tools/RHEL5/bin/qmake -o $@ $<
	
	
