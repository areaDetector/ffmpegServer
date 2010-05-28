#ifndef ffmpegFile_H
#define ffmpegFile_H

#include "NDPluginFile.h"

/* ffmpeg and null-httpd includes */
extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
}

/** Enums for plugin-specific parameters. */
typedef enum
{
    ffmpegFileBitrate           /* (asynInt32, r/w) File bitrate */
        = NDPluginFileLastParam,
	ffmpegFileFPS,               /* (asynInt32, r/w) Frames per second */    
	ffmpegFileHeight,               /* (asynInt32, r/w) Video Height */  
	ffmpegFileWidth,               /* (asynInt32, r/w) Video Width */  		    
    ffmpegFileLastParam
} ffmpegFileParam_t;


/** Writes NDArrays to a ffmpeg file. This can be one of many video formats
  */
class ffmpegFile : public NDPluginFile {
public:
    ffmpegFile(const char *portName, int queueSize, int blockingCallbacks,
               const char *NDArrayPort, int NDArrayAddr,
               int priority, int stackSize);

    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                        const char **pptypeName, size_t *psize);

    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();

private:
    FILE *outFile;
    AVCodec *codec;
    AVCodecContext *c;
    AVFrame *inPicture;
    AVFrame *scPicture;    
    NDArray *scArray;
    NDArray *outArray;
    struct SwsContext *ctx;      
    int outSize, needStop;      
    int sheight, swidth;
    PixelFormat spix_fmt;
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVStream *video_st;
    double video_pts;   
};

#endif
