< envPaths

dbLoadDatabase "$(TOP)/dbd/example.dbd"
example_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("PREFIX", "13SIM1:")
epicsEnvSet("PORT",   "SIM1")
epicsEnvSet("QSIZE",  "20")
epicsEnvSet("XSIZE",  "1024")
epicsEnvSet("YSIZE",  "1024")
epicsEnvSet("NCHANS", "2048")

# Create a simDetector driver
# simDetectorConfig(const char *portName, int maxSizeX, int maxSizeY, int dataType,
#                   int maxBuffers, int maxMemory, int priority, int stackSize)
simDetectorConfig("$(PORT)", $(XSIZE), $(YSIZE), 1, 0, 0)
dbLoadRecords("$(ADCORE)/db/ADBase.template",     "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
dbLoadRecords("$(ADCORE)/db/simDetector.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")

# Create a standard arrays plugin, set it to get data from the simDetector driver.
NDStdArraysConfigure("Image1", 3, 0, "$(PORT)", 0)
dbLoadRecords("$(ADCORE)/db/NDPluginBase.template","P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0")

# This creates a waveform large enough for 1024x1024x3 (e.g. RGB color) arrays.
# This waveform only allows transporting 8-bit images
dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Int8,FTVL=UCHAR,NELEMENTS=3145728")
# This waveform only allows transporting 16-bit images
#dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Int16,FTVL=USHORT,NELEMENTS=3145728")
# This waveform allows transporting 32-bit images
#dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Int32,FTVL=LONG,NELEMENTS=3145728")

# Load all other plugins using commonPlugins.cmd
< $(ADCORE)/iocBoot/commonPlugins.cmd

ffmpegServerConfigure(8081)
# ffmpegStreamConfigure(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxMemory)
ffmpegStreamConfigure("C1.MJPG", 2, 0, "$(PORT)", "0", -1)
dbLoadRecords("$(FFMPEGSERVER)/db/ffmpegStream.template", "P=FFMPEG:,R=Stream,PORT=C1.MJPG")

# ffmpegFileConfigure(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr)
ffmpegFileConfigure("C1.FILE", 16, 0, "$(PORT)", 0)
dbLoadRecords("$(FFMPEGSERVER)/db/ffmpegFile.template", "P=FFMPEG:,R=Stream,PORT=C1.FILE")

iocInit

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30, "P=$(PREFIX)")
