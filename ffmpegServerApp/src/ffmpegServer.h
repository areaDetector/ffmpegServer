#ifndef ffmpegServer_H
#define ffmpegServer_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

/** maximum number of streams that the http server will host, fairly arbitrary */
#define MAX_FFMPEG_STREAMS 64

#define ffmpegServerQualityString  "FFMPEG_QUALITY"   /* JPEG quality (int32 read/write) */
#define ffmpegServerFalseColString "FFMPEG_FALSE_COL" /* False Colour toggle (int32 (enum) read/write)*/
#define ffmpegServerHttpPortString "FFMPEG_HTTP_PORT" /* Http port (int32 read)*/
#define ffmpegServerHostString     "FFMPEG_HOST"      /* Host string (string read)*/
#define ffmpegServerJpgUrlString   "FFMPEG_JPG_URL"   /* JPEG Snapshot URL string (string read)*/
#define ffmpegServerMjpgUrlString  "FFMPEG_MJPG_URL"  /* MJPG Stream URL string (string read)*/
#define ffmpegServerClientsString  "FFMPEG_CLIENTS"   /* Number of connected clients (int32 read)*/
#define ffmpegServerAlwaysOnString "FFMPEG_ALWAYS_ON" /* Always produce jpeg, even when no-one is listening (int32 read)*/

/** Take an array source and compress it and serve it as an mjpeg stream. */
class ffmpegStream : public NDPluginDriver {
public:
    ffmpegStream(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr, int maxBuffers, int maxMemory,
                 int priority, int stackSize);                
    /* These methods override those in the base class */
    void processCallbacks(NDArray *pArray);
    /* These are used by the http server to access the data in this class */
    int send_frame(int sid, NDArray *pArray);
    void send_stream(int sid);
    void send_snapshot(int sid, int index);   

protected:
    int ffmpegServerQuality;
    #define FIRST_FFMPEG_SERVER_PARAM ffmpegServerQuality
    int ffmpegServerFalseCol;
    int ffmpegServerHttpPort;
    int ffmpegServerHost;
    int ffmpegServerJpgUrl;
    int ffmpegServerMjpgUrl;            
    int ffmpegServerClients;
    int ffmpegServerAlwaysOn;
    #define LAST_FFMPEG_SERVER_PARAM ffmpegServerAlwaysOn
                                
private:
    NDArray *scArray;    
    NDArray *jpeg;
    int nclients;
         
    AVCodec *codec;
    AVCodecContext *c;
    AVFrame *inPicture;
    AVFrame *scPicture;            
    struct SwsContext *ctx;      
    
    /* signal fresh frames */
    pthread_cond_t *cond;
    pthread_mutex_t mutex;    

    NDArray* get_jpeg();
    NDArray* wait_for_jpeg(int sid);    
    void allocScArray(int size);
};
#define NUM_FFMPEG_SERVER_PARAMS (&LAST_FFMPEG_SERVER_PARAM - &FIRST_FFMPEG_SERVER_PARAM + 1)                             
                             
/* These are global to the http server */
static ffmpegStream **streams;
static int nstreams;
    
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
