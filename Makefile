#Makefile at top of application tree
TOP = .

include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), vendor)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *App))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *app))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocBoot))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocboot))

# Comment out the following lines to disable creation of example iocs
# This requires the use of the Diamond iocbuilder python software
#DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard etc))
#ifeq ($(wildcard etc),etc)
#	include $(TOP)/etc/makeIocs/Makefile.iocs
#	UNINSTALL_DIRS += documentation/doxygen $(IOC_DIRS)
#endif

# make sure Viewer is only built on linux-x86
ifeq ($(EPICS_HOST_ARCH), linux-x86)
    # This builds the QT viewer
    DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard Viewers))
    install: $(TOP)/Viewers/Makefile
    clean: $(TOP)/Viewers/Makefile        
endif

# Comment out the following line to disable building of example iocs
#DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocs))

include $(TOP)/configure/RULES_TOP

$(TOP)/Viewers/Makefile: $(TOP)/Viewers/Viewers.pro
	/dls_sw/prod/tools/RHEL5/bin/qmake -o Makefile $<	
