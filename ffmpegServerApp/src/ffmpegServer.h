#ifndef ffmpegServer_H
#define ffmpegServer_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

/** maximum number of streams that the http server will host, fairly arbitrary */
#define MAX_FFMPEG_STREAMS 64

/** Enums for plugin-specific parameters. */
typedef enum {
    ffmpeg_quality      /** JPEG quality (int32 read/write) */ = NDPluginDriverLastParam,
    ffmpeg_false_col    /** False Colour toggle (int32 (enum) read/write)*/,
    ffmpeg_http_port    /** Http port (int32 read)*/,
    ffmpeg_host         /** Host string (string read)*/,    
    ffmpeg_clients      /** Number of connected clients (int32 read)*/,
    ffmpeg_always_on    /** Always produce jpeg, even when no-one is listening (int32 read)*/,
    ffmpegServerLastParam
} ffmpegServerParam_t;

/** The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static asynParamString_t ffmpegServerParamString[] = {
    {ffmpeg_quality,     "FFMPEG_QUALITY"     },
    {ffmpeg_false_col,   "FFMPEG_FALSE_COL"   },
    {ffmpeg_http_port,   "FFMPEG_HTTP_PORT"   },
    {ffmpeg_host,        "FFMPEG_HOST"        },    
    {ffmpeg_clients,     "FFMPEG_CLIENTS"     },    
    {ffmpeg_always_on,   "FFMPEG_ALWAYS_ON"   }            
};
#define NUM_FFMPEG_SERVER_PARAMS (sizeof(ffmpegServerParamString)/sizeof(ffmpegServerParamString[0]))

/** Take an array source and compress it and serve it as an mjpeg stream.
  * Can also do false colour and grid
  */
class ffmpegStream : public NDPluginDriver {
public:
    ffmpegStream(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr, int maxBuffers, int maxMemory);
    NDArray *processedArray;    
    NDArray *neutral;   
    NDArray *jpeg;
    int nclients;
         
    AVCodec *codec;
    AVCodecContext *c;
    AVFrame *picture;    
    
    /* signal fresh frames */
    pthread_cond_t *cond;
    pthread_mutex_t mutex;    

	int send_frame(int sid, NDArray *pArray);
	void send_stream(int sid);
	void send_snapshot(int sid, int index);
	NDArray* get_jpeg();
	NDArray* wait_for_jpeg(int sid);	
	void allocProcessedArray(int size);
                 
    /* These methods override those in the base class */
    void processCallbacks(NDArray *pArray);
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                             const char **pptypeName, size_t *psize);                             
};

/* These are global to the http server */
static ffmpegStream **streams;
static int nstreams;
    
#endif
