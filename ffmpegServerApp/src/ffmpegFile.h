#ifndef ffmpegFile_H
#define ffmpegFile_H

#include "NDPluginFile.h"
#include "ffmpegCommon.h"
#ifdef WIN32
#define snprintf _snprintf
#endif

#define ffmpegFileBitrateString "FFMPEG_BITRATE"  /* (asynFloat64, r/w) File bitrate */
#define ffmpegFileFPSString     "FFMPEG_FPS"      /* (asynInt32, r/w) Frames per second */
#define ffmpegFileHeightString  "FFMPEG_HEIGHT"   /* (asynInt32, r/w) Video Height */
#define ffmpegFileWidthString   "FFMPEG_WIDTH"    /* (asynInt32, r/w) Video Width */

/* Minimum integer exactly representable in IEEE 754 double (-2^53) */
#define EXACT_INT_DBL_MIN -9007199254740992
/* Maximum integer exactly representable in IEEE 754 double (2^53) */
#define EXACT_INT_DBL_MAX 9007199254740992

/** Writes NDArrays to a ffmpeg file. This can be one of many video formats
  */
class ffmpegFile : public NDPluginFile {
public:
    ffmpegFile(const char *portName, int queueSize, int blockingCallbacks,
               const char *NDArrayPort, int NDArrayAddr,
               int priority, int stackSize);
    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

protected:
    int ffmpegFileBitrate;
    #define FIRST_FFMPEG_FILE_PARAM ffmpegFileBitrate
    int ffmpegFileFPS;
    int ffmpegFileHeight;
    int ffmpegFileWidth;
    #define LAST_FFMPEG_FILE_PARAM ffmpegFileWidth

private:
    FILE *outFile;
    AVCodec *codec;
    enum AVCodecID codec_id;
    AVCodecContext *c;
    AVFrame *inPicture;
    AVFrame *scPicture;    
    NDArray *scArray;
    NDArray *outArray;
    struct SwsContext *ctx;      
    size_t outSize;
    int needStop;      
    int sheight, swidth;
    enum AVPixelFormat spix_fmt;
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVStream *video_st;
    double video_pts;   
};
#define NUM_FFMPEG_FILE_PARAMS (int)(&LAST_FFMPEG_FILE_PARAM - &FIRST_FFMPEG_FILE_PARAM + 1)   

#endif
/**
 * \file
 * \section License
 * Author: Diamond Light Source, Copyright 2010
 *
 * 'ffmpegServer' is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * 'ffmpegServer' is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with 'ffmpegServer'.  If not, see http://www.gnu.org/licenses/.
 */
