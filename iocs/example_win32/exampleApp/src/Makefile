TOP=../..

include $(TOP)/configure/CONFIG

# ------------------------
# Build an IOC Application
# ------------------------

PROD_IOC = example

# example.dbd will be installed into <top>/dbd
DBD += example.dbd

# example.dbd will be created from these files
example_DBD += base.dbd
example_DBD += asyn.dbd
example_DBD += ADSupport.dbd
example_DBD += pluginSupport.dbd
example_DBD += busySupport.dbd
example_DBD += prosilicaSupport.dbd
example_DBD += ffmpegServer.dbd

# example_registerRecordDeviceDriver.cpp will be created
# example.dbd
example_SRCS += example_registerRecordDeviceDriver.cpp

# These two lines are needed for non-vxWorks builds, such as Linux
example_SRCS += exampleMain.cpp

# This line says that this IOC Application depends on the
example_LIBS += prosilica
example_LIBS += NDPlugin ADBase netCDF PvAPI ImageLib
example_LIBS += calc busy sscan asyn
example_LIBS += ffmpegServer avcodec swscale avutil
# Windows libraries supplied by areaDetector
example_LIBS_WIN32 += jpeg libNeXus.dll

# Copy dlls across
BIN_INSTALLS_WIN32 += $(wildcard $(AREADETECTOR)/bin/win32-x86/*.dll)
BIN_INSTALLS_WIN32 += $(wildcard $(FFMPEG)/bin/win32-x86/*.dll)

# We need to link this IOC Application against the EPICS Base libraries
example_LIBS += $(EPICS_BASE_IOC_LIBS)

# ---------------------------------------------------

include $(TOP)/configure/RULES

