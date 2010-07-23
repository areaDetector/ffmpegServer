#define __STDC_CONSTANT_MACROS
#include <stdint.h>

/* ffmpeg and null-httpd includes */
extern "C" {
#include "nullhttpd.h"
#include "libavcodec/avcodec.h"
}

/* local includes */
#include "ffmpegServer.h"
#include "conversionTables.h"

/* EPICS includes */
#include "epicsExport.h"
#include "iocsh.h"
#include <epicsExit.h>

/* windows includes */
#ifdef _MSC_VER /* Microsoft Compilers */
//#define _WIN32_WINNT 0x500 /* WINBASE.H - Enable SignalObjectAndWait */
int pthread_cond_init (pthread_cond_t *cv, const pthread_condattr_t *) {
  cv->semaphore = CreateEvent (NULL, FALSE, FALSE, NULL);
  return 0;
}

int pthread_cond_wait (pthread_cond_t *cv, pthread_mutex_t *external_mutex) {
  pthread_mutex_unlock(external_mutex);
  WaitForSingleObject (cv->semaphore, INFINITE);
  pthread_mutex_lock(external_mutex);
  return 0;
}

int pthread_cond_signal (pthread_cond_t *cv) {
  SetEvent (cv->semaphore);
  return 0;
}
#endif

static const char *driverName = "ffmpegServer";

/* This is called whenever a client requests a stream */
void dorequest(int sid) {
    char *token;
    
    if (read_header(sid)<0) {
        closeconnect(sid, 1);
        return;
    }
    logaccess(2, "%s - HTTP Request: %s %s", conn[sid].dat->in_RemoteAddr, 
            conn[sid].dat->in_RequestMethod, conn[sid].dat->in_RequestURI);
    snprintf(conn[sid].dat->out_ContentType, 
            sizeof(conn[sid].dat->out_ContentType)-1, "text/html");

    /* check if we are asking for a jpg or an mjpg file */
    token = strtok(conn[sid].dat->in_RequestURI+1, ".");

    if (token != NULL) {
        for (int i=0; i<nstreams; i++) {
            if (strcmp(token, streams[i]->portName) == 0) {
                token = strtok(NULL, ".");
                if (strcmp(token, "index") == 0) {
                    streams[i]->send_snapshot(sid, 1);
                    return;
                }                
                if (strcmp(token, "jpg") == 0 || strcmp(token, "jpeg") == 0) {
                    streams[i]->send_snapshot(sid, 0);
                    return;
                }
                if (strcmp(token, "mjpg") == 0 || strcmp(token, "mjpeg") == 0) {                    
                    streams[i]->send_stream(sid);
                    return;
                }

            }
        }
    }

    /* If we weren't asked for a stream then just send the index page */
    send_header(sid, 0, 200, "OK", "1", "text/html", -1, -1);
    prints("\n\
<HTML> \n\
 <style type=\"text/css\"> \n\
BODY {       \n\
  background-color: #f2f2f6; \n\
  font-family: arial, verdana, helvetica, sans-serif; \n\
} \n\
H1 { \n\
  font-size: 24px; \n\
  color: #000000; \n\
  font-weight: bold; \n\
  text-align: center; \n\
} \n\
H2 { \n\
  font-size: 18px; \n\
  color: #1b2a60; \n\
  font-weight: bold; \n\
  text-align: center; \n\
} \n\
A:link { \n\
  text-decoration: none; color:#3b4aa0; \n\
} \n\
A:visited { \n\
  text-decoration: none; color:#3b4aa0; \n\
} \n\
A:active { \n\
  text-decoration: none; color:#3b4aa0; \n\
} \n\
A:hover { \n\
  text-decoration: underline; color:#3b4aff; \n\
} \n\
table { \n\
  border-spacing: 25px; \n\
} \n\
td { \n\
  background-color: #e0e0ee; \n\
  border-style: outset; \n\
  border-color: #e0e0ee; \n\
  border-width: 1px; \n\
  padding: 10px; \n\
  text-align: center; \n\
} \n\
p { \n\
  text-align: center; \n\
} \n\
 </style> \n\
 <HEAD> \n\
  <TITLE>ffmpegServer Content Listing</TITLE> \n\
 </HEAD> \n\
 <BODY> \n\
  <CENTER> \n\
   <H1>ffmpegServer Content Listing</H1> \n\
   <TABLE> \n\
    <TR> \n\
");    
    /* make a table of streams */
    for (int i=0; i<nstreams; i++) {
        if (i % 4 == 3) { //4 per row
            prints("    </TR>\n    <TR>\n");                    
        }
        prints("<TD>\n");
        prints("<H2>%s</H2>\n", streams[i]->portName);
        prints("<img src=\"%s.index.jpg\" height=\"192\" title=\"Static JPEG\" alt=\"&lt;No image yet&gt;\"/>\n", streams[i]->portName);        
        prints("<p><a href=\"%s.jpg\">Link to Static JPEG</a></p>\n", streams[i]->portName);
        prints("<p><a href=\"%s.mjpg\">Link to MJPEG Stream</a></p>\n", streams[i]->portName);        
        prints("</TD>\n");        
    }
    prints("\n\
    </TR> \n\
   </TABLE> \n\
  </CENTER> \n\
 </BODY> \n\
</HTML> \n\
");
    flushbuffer(sid);    
}

/* this dummy function is here to satisfy nullhttpd */
int config_read()
{
    snprintf(config.server_base_dir, sizeof(config.server_base_dir)-1, "%s", DEFAULT_BASE_DIR);
    snprintf(config.server_bin_dir, sizeof(config.server_bin_dir)-1, "%s/bin", config.server_base_dir);
    snprintf(config.server_cgi_dir, sizeof(config.server_cgi_dir)-1, "%s/cgi-bin", config.server_base_dir);
    snprintf(config.server_etc_dir, sizeof(config.server_etc_dir)-1, "%s/etc", config.server_base_dir);
    snprintf(config.server_htdocs_dir, sizeof(config.server_htdocs_dir)-1, "%s/htdocs", config.server_base_dir);
    fixslashes(config.server_base_dir);
    fixslashes(config.server_bin_dir);
    fixslashes(config.server_cgi_dir);
    fixslashes(config.server_etc_dir);
    fixslashes(config.server_htdocs_dir);
    return 0;
}

static int stopping = 0;

void c_shutdown(void *) {
    printf("Shutting down http server...");
    stopping = 1;
    server_shutdown();
    sleep(1);
    printf("OK\n");
}    

/* configure and start the http server */
void ffmpegServerConfigure(int port) {
    int status;
    if (port==0) {
        port = 8080;
    }    
    streams = (ffmpegStream **) calloc(MAX_FFMPEG_STREAMS, sizeof(ffmpegStream *));
    nstreams = 0;    
    config.server_port = port;
    config.server_loglevel=1;
    strncpy(config.server_hostname, "any", sizeof(config.server_hostname)-1);
    config.server_maxconn=50;
    config.server_maxidle=120;    
    printf("Starting server on port %d...\n", port);
    snprintf((char *)program_name, sizeof(program_name)-1, "ffmpegServer");
    init();
#ifdef WIN32
    if (_beginthread(WSAReaper, 0, NULL)==-1) {
        logerror("Winsock reaper thread failed to start");
        exit(0);
    }
#endif
    /* Start up acquisition thread */
    status = (epicsThreadCreate("httpServer",
            epicsThreadPriorityMedium,
            epicsThreadGetStackSize(epicsThreadStackMedium),
            (EPICSTHREADFUNC)accept_loop,
            NULL) == NULL);
    if (status) {
        printf("%s:ffmpegServerConfigure epicsThreadCreate failure for image task\n",
                driverName);
        return;
    } else printf("OK\n");
    /* Register the shutdown function for epicsAtExit */
    epicsAtExit(c_shutdown, NULL);    
}

/* this sends a single snapshot */
void ffmpegStream::send_snapshot(int sid, int index) {
    time_t now=time((time_t*)0);
    int size, always_on;    
    NDArray* pArray;
//  printf("JPEG requested\n");
    /* Say we're listening */
    getIntegerParam(0, ffmpegServerAlwaysOn, &always_on);
    pthread_mutex_lock( &this->mutex );    
    this->nclients++;    
    pthread_mutex_unlock(&this->mutex);
    /* if always on or clients already listening then there is already a frame */        
    if (always_on || this->nclients > 1 || index) {
        pArray = get_jpeg();
    } else {
        pArray = wait_for_jpeg(sid);
    }    
    /* we're no longer listening */
    pthread_mutex_lock( &this->mutex );    
    this->nclients--;    
    pthread_mutex_unlock(&this->mutex);    
    /* If there's no data yet, say so */
    if (pArray == NULL) {
        printerror(sid, 200, "No Data", "No jpeg available yet");
        return;
    }
    /* Send the right header for a jpeg */
    size = pArray->dims[0].size;    
    conn[sid].dat->out_ContentLength=size;
    send_fileheader(sid, 0, 200, "OK", "1", "image/jpeg", size, now);
    /* Send the jpeg itself */
    send(conn[sid].socket, (const char *) pArray->pData, size, 0);
    pArray->release();
    /* Clear up */
    conn[sid].dat->out_headdone=1;
    conn[sid].dat->out_bodydone=1;
    conn[sid].dat->out_flushed=1;
    conn[sid].dat->out_ReplyData[0]='\0';
    flushbuffer(sid);
}

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))


/* This sends a jpeg frame as part of an mjpeg stream */
int ffmpegStream::send_frame(int sid, NDArray *pArray) {
    int ret = 0;
    if (pArray) {
        /* Send metadata */
//        printf("Send frame %d to sid %d\n", pArray->dims[0].size, sid);                        
        prints("Content-Type: image/jpeg\r\n");
        prints("Content-Length: %d\r\n\r\n", pArray->dims[0].size);
        flushbuffer(sid);
        /* Send the jpeg */
        for (int i=0; i<pArray->dims[0].size; i+=512) {
            ret = send(conn[sid].socket, ((const char *) pArray->pData) + i, MIN(512,pArray->dims[0].size-i), 0);
            if (ret<0) return ret;
            flushbuffer(sid);             
        }                   
        /* Send a boundary */
        prints("\r\n--BOUNDARY\r\n");
        flushbuffer(sid);
//        printf("Done\n");        
        pArray->release();             
    }
    return ret;
}    

/** Get the current jpeg and return it */
NDArray* ffmpegStream::get_jpeg() {
    NDArray* pArray;
    pthread_mutex_lock(&this->mutex);
    pArray = this->jpeg;
    if (pArray) pArray->reserve();  
    pthread_mutex_unlock(&this->mutex);
    return pArray;
}    
    

/** Wait for a jpeg to be produced */
NDArray* ffmpegStream::wait_for_jpeg(int sid) {
    NDArray* pArray;
    pthread_cond_wait(&this->cond[sid], &this->mutex);
    pArray = this->jpeg;
    pArray->reserve();  
    pthread_mutex_unlock(&this->mutex);   
    return pArray;    
}    


void ffmpegStream::send_stream(int sid) {
    int ret = 0;
    int always_on;
    NDArray* pArray;
    time_t now=time((time_t*)0);    
    /* Say we're listening */
    getIntegerParam(0, ffmpegServerAlwaysOn, &always_on);
    pthread_mutex_lock( &this->mutex );    
    this->nclients++;    
    pthread_mutex_unlock(&this->mutex);    
    /* Send the appropriate header */
    send_fileheader(sid, 0, 200, "OK", "1", "multipart/x-mixed-replace;boundary=BOUNDARY", -1, now);
    prints("--BOUNDARY\r\n");
    flushbuffer(sid);
    /* if always on or clients already listening then there is already a frame */    
    if (always_on || this->nclients > 1) {
        pArray = get_jpeg();
        ret = send_frame(sid, pArray);    
    }
    /* while the client is listening and we aren't stopping */
    while (ret >= 0 && !stopping) {
        /* wait for a new frame and send it*/
        pArray = wait_for_jpeg(sid);
        ret = send_frame(sid, pArray);
    }
    /* We're no longer listening */
    pthread_mutex_lock( &this->mutex );    
    this->nclients--;    
    pthread_mutex_unlock(&this->mutex);            
} 

/* alloc the processed array */
void ffmpegStream::allocProcessedArray(int size) {
    if (this->processedArray) {
        if (this->processedArray->dims[0].size >= size) {
            /* the processed array is already big enough */
            return;
        } else {
            /* need a new one, so discard the old one */
            this->processedArray->release();
        }
    }
    this->processedArray = this->pNDArrayPool->alloc(1, &size, NDInt8, 0, NULL);
}       
    

/** Take an NDArray, add grid and false colour, compress it to a jpeg, then
 * signal to the server process that there is a new frame available.
 */
void ffmpegStream::processCallbacks(NDArray *pArray)
{
    /* we're going to get these with getIntegerParam */
    int quality, clients, false_col, always_on;
    /* we're going to get these from the dims of the image */
    int width, height;
    /* these are just temporary vars */
    int halfsize, size, x, y, i, i2, j, uoff, voff, twoOff;
    /* these are for getting the colour mode */
    int colorMode = NDColorModeMono;
    NDAttribute *pAttribute = NULL;
    /* use this if the array is copied */    
    int useProcessedArray = 0;
    /* set this if the array is yuv instead of grayscale */
    int yuv = 0; 
    /* these are so we can operate on the data char by char */
    unsigned char *destFrame;
    /* for printing errors */
    const char *functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);
    
    /* see if anyone's listening */
    pthread_mutex_lock(&this->mutex);    
    clients = this->nclients;
    pthread_mutex_unlock(&this->mutex);  
    setIntegerParam(0, ffmpegServerClients, clients);
    
    /* get the configuration values */
    getIntegerParam(0, ffmpegServerQuality, &quality);
    getIntegerParam(0, ffmpegServerFalseCol, &false_col);
    getIntegerParam(0, ffmpegServerAlwaysOn, &always_on);    

    /* if no-ones listening and we're not always on then do nothing */
    if (clients == 0 && always_on == 0) {
//        printf("No-one listening\n");
        return;
    }

    /* check it's the right format */
    switch (pArray->dataType) {
        case NDInt8:
        case NDUInt8:
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: only 8-bit data is supported\n",
            driverName, functionName);
        return;
    }

    /* We do some special treatment based on colorMode */
    pAttribute = pArray->pAttributeList->find("ColorMode");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colorMode);

    if ((pArray->ndims == 2) && (colorMode == NDColorModeMono)) {
        /* Mono mode, just use the array as is */
        width = (int) pArray->dims[0].size;
        height = (int) pArray->dims[1].size;
        destFrame = (unsigned char *) pArray->pData;
        yuv = 0;
//        printf("Mono\n");                     
    } else if ((pArray->ndims == 3) && (pArray->dims[0].size == 3) && (colorMode == NDColorModeRGB1)) {
        /* RGB pixel interleave */
        width = (int) pArray->dims[1].size;
        height = (int) pArray->dims[2].size;
//        printf("RGB1\n");        
        yuv = 1;
    } else if ((pArray->ndims == 3) && (pArray->dims[1].size == 3) && (colorMode == NDColorModeRGB2)) {
        /* RGB line interleave */
        width = (int) pArray->dims[0].size;
        height = (int) pArray->dims[2].size;
//        printf("RGB2\n");                
        yuv = 1;
    } else if ((pArray->ndims == 3) && (pArray->dims[2].size == 3) && (colorMode == NDColorModeRGB3)) {
        /* RGB plane interleave */
        width = (int) pArray->dims[0].size;
        height = (int) pArray->dims[1].size;
//        printf("RGB3\n");                
        yuv = 1;
    } else {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: unsupported array structure\n",
            driverName, functionName);
        return;
    }
    size = width * height;

    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing memory
     * that other threads can access. */
    this->unlock();

/* from http://www.fourcc.org/fccyvrgb.php
 * Ey = 0.299R+0.587G+0.114B
 * Ecb = 0.564(B - Ey) = -0.169R-0.331G+0.500B
 * Ecr = 0.713(R - Ey) = 0.500R-0.419G-0.081B
 */
#define RGB2Y(ri,gi,bi,j) destFrame[(j)] = (unsigned char) (\
    conv_0299[srcFrame[(ri)]] + \
    conv_0587[srcFrame[(gi)]] + \
    conv_0114[srcFrame[(bi)]] );
#define BY2U(bi,bi2,j) destFrame[uoff + (j) / 2] = (unsigned char) (0.282 * \
  (srcFrame[(bi)] + srcFrame[(bi2)] - destFrame[(j)] - destFrame[(j)+1]) + 128);
#define RY2V(ri,ri2,j) destFrame[voff + (j) / 2] = (unsigned char) (0.357 * \
  (srcFrame[(ri)] + srcFrame[(ri2)] - destFrame[(j)] - destFrame[(j)+1]) + 128);
    
    /* Convert an rgb frame to yuv */
    if (yuv) {
        /* make sure our processed array is big enough */
        this->allocProcessedArray(2 * size);
        useProcessedArray = 1;
        unsigned char *srcFrame = (unsigned char *) pArray->pData;
        destFrame = (unsigned char *) this->processedArray->pData;
        uoff = size;
        voff = uoff * 3 / 2;
        /* Convert it to a frame that we can do YUV422 conversion on it
         * Y should be twice the length of U and V
         * i = index of first red source pixel
         * i2 = index of second red source pixel
         */
        switch (colorMode) {
            case NDColorModeRGB1:
                /* RGB pixel interleave */
                for (j=0;j<size;j+=2) {
                    i = 3*j;
                    /* if i is not last pixel in array, use next pixel
                     * else use i */
                    i2 = j+1<size?i+3:i;
                    RGB2Y(i, i+1, i+2, j); // Y0
                    RGB2Y(i2, i2+1, i2+2, j+1); // Y1
                    /* Only make U and V if we don't want false colour */
                    if (false_col==0) {
                        BY2U(i+2, i2+2, j); // U
                        RY2V(i, i2, j); // V
                    }
                }
                break;
            case NDColorModeRGB2:
                /* RGB line interleave */
                twoOff = 2*width;
                for (j=0;j<size;j+=2) {
                    x = j % width;
                    y = j / width;
                    i = y*width*3 + x;
                    /* if i is not last pixel in line, use next pixel
                      * elif i is not last pixel in array, use next line
                      * else use i */                        
                    i2 = x+1<width?i+1:y+1<height?y+1*width*3:i;
                    RGB2Y(i, i+width, i+twoOff, j); // Y0
                    RGB2Y(i2, i2+width, i2+twoOff, j+1); // Y1
                    /* Only make U and V if we don't want false colour */
                    if (false_col==0) {
                        BY2U(i+twoOff, i2+twoOff, j); // U
                        RY2V(i, i2, j); //V
                    }
                }
                break;                
            case NDColorModeRGB3:
                /* RGB plane interleave */
                twoOff = 2*size;
                for (j=0;j<size;j+=2) {
                    i = j;
                    /* if i is not last pixel in array, use next pixel
                     * else use i */
                    i2 = j+1<size?i+1:i;
                    RGB2Y(i, i+size, i+twoOff, j); // Y0
                    RGB2Y(i2, i2+size, i2+twoOff, j+1); //Y1
                    /* Only make U and V if we don't want false colour */
                    if (false_col==0) {
                        BY2U(i+twoOff, i2+twoOff, j); // U
                        RY2V(i, i2, j); // V
                    }
                }
                break;                               
        }
    }                

    /* make the image false colour */
    if (false_col!=0) {
        if (useProcessedArray==0) {
            /* make sure our processed array is big enough */
            this->allocProcessedArray(2 * size);
            memcpy(this->processedArray->pData, pArray->pData, size);        
            useProcessedArray = 1;
        }
        destFrame = (unsigned char *) this->processedArray->pData;
        uoff = size;
        voff = uoff * 3 / 2;
        yuv = 1;
        for (i=0;i<size;i+=2) {
            /* if i is not last pixel in array, use next pixel
                * else use i */
            i2 = i+1<size?i+1:i;        
            /* Take the average of the 2 Y points */
            j = (destFrame[i] + destFrame[i2]) / 2;
            destFrame[uoff + i / 2] = false_Cb[j];
            destFrame[voff + i / 2] = false_Cr[j];
            /* do the false y pixels */            
            destFrame[i] = false_Y[destFrame[i]];
            destFrame[i2] = false_Y[destFrame[i2]];
        }
    }
    
    /* lock the output plugin mutex */
    pthread_mutex_lock(&this->mutex);
    
    /* Set the quality */    
    picture->quality = 3276 - (int) (quality * 32.76);
    if (picture->quality < 0) picture->quality = 0;
    if (picture->quality > 32767) picture->quality = 32768;    
    
    /* Release the last jpeg created */
    if (this->jpeg) {
        this->jpeg->release();
    }

    /* If width and height have changed then reinitialise the codec */
    halfsize = size / 2;
    if (c == NULL || width != c->width || height != c->height) {
//        printf("Setting width %d height %d\n", width, height);
        AVRational avr;
        avr.num = 1;
        avr.den = 25;
        if (c) {
            /* width and height changed, close old codec */
            avcodec_close(c);
            av_free(c);
        }
        c = avcodec_alloc_context();
        c->width = width;
        c->height = height;
        c->pix_fmt = PIX_FMT_YUVJ422P;
        c->flags = CODEC_FLAG_QSCALE;
        c->time_base = avr;
        /* open it */
        if (avcodec_open(c, codec) < 0) {
            c = NULL;
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s: could not open codec\n",
                driverName, functionName);                
            return;
        }
        picture->linesize[0] = c->width;
        picture->linesize[1] = c->width / 2;
        picture->linesize[2] = c->width / 2;
    }
    /* setup linesize and data pointers */
    if (yuv==0) {
        /* Greyscale, format as YUV422P with U and V neutral colours */
        picture->data[0] = destFrame;
        /* need our neutral array */
        if (this->neutral==NULL || this->neutral->dims[0].size < size / 2) {
            if (this->neutral) {
                this->neutral->release();
            }
            this->neutral = this->pNDArrayPool->alloc(1, &halfsize, NDInt8, 0, NULL);
            memset(this->neutral->pData, 128, halfsize);            
        }        
        picture->data[1] = (uint8_t*) this->neutral->pData;
        picture->data[2] = (uint8_t*) this->neutral->pData;
    } else {
        /* YUV422P data */
        picture->data[0] = destFrame;
        picture->data[1] = destFrame + size;
        picture->data[2] = destFrame + (3 * halfsize);
    }    
    
    /* Convert it to a jpeg */        
    this->jpeg = this->pNDArrayPool->alloc(1, &size, NDInt8, 0, NULL);
    this->jpeg->dims[0].size = avcodec_encode_video(c, (uint8_t*)this->jpeg->pData, c->width * c->height, picture);    
//    printf("Frame! Size: %d\n", this->jpeg->dims[0].size);
    
    /* signal fresh_frame to output plugin and unlock mutex */
    for (int i=0; i<config.server_maxconn; i++) {
        pthread_cond_signal(&(this->cond[i]));
    }
    pthread_mutex_unlock(&this->mutex);

    /* We must enter the loop and exit with the mutex locked */
    this->lock();

    /* Update the parameters.  */
    callParamCallbacks(0, 0);
}

/** The constructor for this class
 *
 * - portName = asyn port name of this driver
 * - queueSize = size of the input queue (in number of NDArrays), normally 1
 * - blockingCallbacks = if 1 then block while processing, normally 0
 * - NDArrayport = asyn port name of the image source (driver or ROI)
 * - httpPort = port number to server ffmpeg stream on
 * - wwwPath = path to html files to serve up
 */
ffmpegStream::ffmpegStream(const char *portName, int queueSize, int blockingCallbacks,
                           const char *NDArrayPort, int NDArrayAddr, int maxBuffers, int maxMemory,
                           int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_FFMPEG_SERVER_PARAMS, maxBuffers, maxMemory,
                   asynGenericPointerMask,
                   asynGenericPointerMask,
                   0, 1, priority, stackSize)  /* Not ASYN_CANBLOCK or ASYN_MULTIDEVICE, do autoConnect */
{
    char host[64] = "";
    asynStatus status;
    this->neutral = NULL;
    this->jpeg = NULL;
    this->processedArray = NULL;

    /* Create some parameters */
    createParam(ffmpegServerQualityString,  asynParamInt32, &ffmpegServerQuality);
    createParam(ffmpegServerFalseColString, asynParamInt32, &ffmpegServerFalseCol);
    createParam(ffmpegServerHttpPortString, asynParamInt32, &ffmpegServerHttpPort);
    createParam(ffmpegServerHostString,     asynParamOctet, &ffmpegServerHost);
    createParam(ffmpegServerClientsString,  asynParamInt32, &ffmpegServerClients);
    createParam(ffmpegServerAlwaysOnString, asynParamInt32, &ffmpegServerAlwaysOn);

    /* Try to connect to the NDArray port */
    status = connectToArrayPort();

    /* Set the initial values of some parameters */
    setIntegerParam(0, ffmpegServerHttpPort, config.server_port);
    setIntegerParam(0, ffmpegServerClients, 0);    
    
    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "ffmpegServer");    

    /* Set the hostname */
    gethostname(host, 64);
    setStringParam(ffmpegServerHost, host);   

    /* must be called before using avcodec lib */
    avcodec_init();

    /* register all the codecs */
    avcodec_register_all();

    /* Setup correct codec for mjpeg */
    codec = avcodec_find_encoder(CODEC_ID_MJPEG);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }
    
    /* Create an AVFrame to encode into */
    picture = avcodec_alloc_frame();

    /* this mutex and the conditional variable are used to synchronize access to the global picture buffer */
    pthread_mutex_init(&(this->mutex), NULL);
    this->cond = (pthread_cond_t *) calloc(config.server_maxconn, sizeof(pthread_cond_t));
    for (int i=0; i<config.server_maxconn; i++) {
        pthread_cond_init(&(this->cond[i]), NULL);
    }
}

/** Configuration routine.  Called directly, or from the iocsh function in ffmpegStreamRegister, calls ffmpegStream constructor*/
extern "C" int ffmpegStreamConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                  const char *NDArrayPort, int NDArrayAddr, int maxBuffers, int maxMemory,
                                  int priority, int stackSize)
{
    if (nstreams+1 > MAX_FFMPEG_STREAMS) {
        printf("%s:ffmpegStreamConfigure: Can only create %d streams\n",
            driverName, MAX_FFMPEG_STREAMS);
        return(asynError);        
    }
    streams[nstreams++] = new ffmpegStream(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg streamArg0 = { "portName",iocshArgString};
static const iocshArg streamArg1 = { "frame queue size",iocshArgInt};
static const iocshArg streamArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg streamArg3 = { "NDArray Port",iocshArgString};
static const iocshArg streamArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg streamArg5 = { "Max Buffers",iocshArgInt};
static const iocshArg streamArg6 = { "Max memory",iocshArgInt};
static const iocshArg streamArg7 = { "priority",iocshArgInt};
static const iocshArg streamArg8 = { "stackSize",iocshArgInt};
static const iocshArg * const streamArgs[] = {&streamArg0,
                                            &streamArg1,
                                            &streamArg2,
                                            &streamArg3,
                                            &streamArg4,
                                            &streamArg5,
                                            &streamArg6,
                                            &streamArg7,                                            
                                            &streamArg8};                                            

static const iocshFuncDef streamFuncDef = {"ffmpegStreamConfigure",9,streamArgs};

static void streamCallFunc(const iocshArgBuf *args)
{
    ffmpegStreamConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival, args[7].ival, args[8].ival);
}

static const iocshArg serverArg0 = { "Http Port",iocshArgInt};
static const iocshArg * const serverArgs[] = {&serverArg0};
static const iocshFuncDef serverFuncDef = {"ffmpegServerConfigure",1,serverArgs};

static void serverCallFunc(const iocshArgBuf *args)
{
    ffmpegServerConfigure(args[0].ival);
}

/** Register ffmpegStreamConfigure and ffmpegServerConfigure for use on iocsh */
extern "C" void ffmpegServerRegister(void)
{
    iocshRegister(&streamFuncDef,streamCallFunc);
    iocshRegister(&serverFuncDef,serverCallFunc);
}

extern "C" {
epicsExportRegistrar(ffmpegServerRegister);
}
