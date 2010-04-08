#Makefile at top of application tree
TOP = .

include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *App))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *app))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocBoot))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocboot))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard etc))
# Comment out the following line to disable building of example iocs
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocs))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard ffmpegViewer))

install: ffmpegViewer/Makefile ffmpegViewer/ffmpegViewer

include $(TOP)/configure/RULES_TOP

ffmpegViewer/Makefile: configure/RELEASE
	/dls_sw/prod/tools/RHEL5/bin/qmake -unix -o $@ "INCLUDEPATH += $(FFMPEG)/include" "LIBPATH += $(FFMPEG)/lib/linux-x86" ffmpegViewer/ffmpegViewer.pro

ffmpegViewer/ffmpegViewer:
	touch $@
