/* local includes */
#include "ffmpegServer.h"

/* EPICS includes */
#include "epicsExport.h"
#include "iocsh.h"
#include <epicsExit.h>
#include <epicsThread.h>
#include <time.h>
#include <math.h>

/* windows includes */
#ifdef _MSC_VER /* Microsoft Compilers */
/** win32 implementation of pthread_cond_init */
int pthread_cond_init (pthread_cond_t *cv, const pthread_condattr_t *) {
  cv->semaphore = CreateEvent (NULL, FALSE, FALSE, NULL);
  return 0;
}
/** win32 implementation of pthread_cond_wait */
int pthread_cond_wait (pthread_cond_t *cv, pthread_mutex_t *external_mutex) {
  pthread_mutex_unlock(external_mutex);
  /** This is wrong, we could miss a wakeup call here... */
  /** Note: this a wait with timeout, on windows we will get a jpeg every second
    * even if there isn't a new one. This avoids GDA timeouts... */
  WaitForSingleObject (cv->semaphore, 1000);
  pthread_mutex_lock(external_mutex);
  return 0;
}
/** win32 implementation of pthread_cond_signal */
int pthread_cond_signal (pthread_cond_t *cv) {
  SetEvent (cv->semaphore);
  return 0;
}
#endif

static const char *driverName = "ffmpegServer";

/** This is called whenever a client requests a stream */
void dorequest(int sid) {
    char *portName;
    int len;
    char *ext;
    
    if (read_header(sid)<0) {
        closeconnect(sid, 1);
        return;
    }
    logaccess(2, "%s - HTTP Request: %s %s", conn[sid].dat->in_RemoteAddr, 
            conn[sid].dat->in_RequestMethod, conn[sid].dat->in_RequestURI);
    snprintf(conn[sid].dat->out_ContentType, 
            sizeof(conn[sid].dat->out_ContentType)-1, "text/html");

    /* check if we are asking for a jpg or an mjpg file */
    ext = strrchr(conn[sid].dat->in_RequestURI+1, '.');

    if (ext != NULL) {
        len = (int) (ext - conn[sid].dat->in_RequestURI - 1);
        portName = (char *)calloc(sizeof(char), 256);
        strncpy(portName, conn[sid].dat->in_RequestURI+1, len);
        ext++;
        for (int i=0; i<nstreams; i++) {
            if (strcmp(portName, streams[i]->portName) == 0) {
                if (strcmp(ext, "index") == 0) {
                    streams[i]->send_snapshot(sid, 1);
                    free(portName);
                    return;
                }                
                if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) {
                    streams[i]->send_snapshot(sid, 0);
                    free(portName);
                    return;
                }
                if (strcmp(ext, "mjpg") == 0 || strcmp(ext, "mjpeg") == 0) {                    
                    streams[i]->send_stream(sid);
                    free(portName);
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
  font-family: arial, sans-serif; \n\
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
   <TABLE cellspacing=\"15\"> \n\
    <TR> \n\
");    
    /* make a table of streams */
    for (int i=0; i<nstreams; i++) {
        prints("<TD>\n");
        prints("<H2>%s</H2>\n", streams[i]->portName);
        prints("<img src=\"%s.index\" height=\"192\" title=\"Static JPEG\" alt=\"&lt;No image yet&gt;\"/>\n", streams[i]->portName);        
        prints("<p><a href=\"%s.jpg\">Link to Static JPEG</a></p>\n", streams[i]->portName);
        prints("<p><a href=\"%s.mjpg\">Link to MJPEG Stream</a></p>\n", streams[i]->portName);        
        prints("</TD>\n");        
        if (i % 3 == 2) { //3 per row
            prints("    </TR>\n    <TR>\n");                    
        }        
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

/** this dummy function is here to satisfy nullhttpd */
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

/** c function that will be called at epicsExit that shuts down the http server cleanly */
void c_shutdown(void *) {
    printf("Shutting down http server...");
    stopping = 1;
    server_shutdown();
    sleep(1);
    printf("OK\n");
}    


/** Configure and start the http server, specifying a specific network interface, or 'any' for default.
 * To specify an interface, use either the DNS name or the IP of the NIC. ex: 10.68.212.33 or my-ioc-server-hostname
 * Default port is 8080. 
 */
void ffmpegServerConfigure(int port, const char* networkInterface) {
    int status;
    if (port==0) {
        port = 8080;
    }

    streams = (ffmpegStream **) calloc(MAX_FFMPEG_STREAMS, sizeof(ffmpegStream *));
    nstreams = 0;    
    config.server_port = port;
    config.server_loglevel=1;
    
    strncpy(config.server_hostname, networkInterface, sizeof(config.server_hostname)-1);
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

/** Internal function to send a single snapshot */
void ffmpegStream::send_snapshot(int sid, int index) {
    time_t now=time((time_t*)0);
    int size, always_on;    
    NDArray* pArray;
//  printf("JPEG requested\n");
    /* Say we're listening */
    getIntegerParam(0, ffmpegServerAlwaysOn, &always_on);
    pthread_mutex_lock( &this->mutex );    
    this->nclients++;   
    if (this->nclients > 1) always_on = 1;
    pthread_mutex_unlock(&this->mutex);
    /* if always on or clients already listening then there is already a frame */        
    if (always_on || index) {
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
    size = (int) pArray->dims[0].size;    
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


/** Internal function to send a jpeg frame as part of an mjpeg stream */
int ffmpegStream::send_frame(int sid, NDArray *pArray) {
    int ret = 0;
//    double difftime;
//    struct timeval start, end;
    if (pArray) {
        /* Send metadata */
//        printf("Send frame %d to sid %d\n", pArray->dims[0].size, sid);                        
        prints("Content-Type: image/jpeg\r\n");
        prints("Content-Length: %d\r\n\r\n", pArray->dims[0].size);
        flushbuffer(sid);
        /* Send the jpeg */
//        gettimeofday(&start, NULL);         
        ret = send(conn[sid].socket, (const char *) pArray->pData, (int) pArray->dims[0].size, 0);                  
//        gettimeofday(&end, NULL);         
        /* Send a boundary */
        prints("\r\n--BOUNDARY\r\n");
        flushbuffer(sid);
//        difftime = (end.tv_usec - start.tv_usec) * 0.000001 + end.tv_sec - start.tv_sec;
//        if (difftime > 0.1) printf ("It took %.2lf seconds to send a frame to %d. That's a bit slow\n", difftime, sid);
//        printf("Done\n");        
        pArray->release();             
    }
    return ret;
}    

/** Internal function to get the current jpeg and return it */
NDArray* ffmpegStream::get_jpeg() {
    NDArray* pArray;
    pthread_mutex_lock(&this->mutex);
    pArray = this->jpeg;
    if (pArray) pArray->reserve();  
    pthread_mutex_unlock(&this->mutex);
    return pArray;
}    
    

/** Internal function to wait for a jpeg to be produced */
NDArray* ffmpegStream::wait_for_jpeg(int sid) {
    NDArray* pArray;
    pthread_mutex_lock(&this->mutex);
    pthread_cond_wait(&(this->cond[sid]), &this->mutex);
    pArray = this->jpeg;
    if(pArray) pArray->reserve();  
    pthread_mutex_unlock(&this->mutex);   
    return pArray;    
}    

/** Internal function to send an mjpg stream */
void ffmpegStream::send_stream(int sid) {
    int ret = 0;
    int always_on;
    NDArray* pArray;
    time_t now=time((time_t*)0);    
    /* Say we're listening */
    getIntegerParam(0, ffmpegServerAlwaysOn, &always_on);
    pthread_mutex_lock( &this->mutex );    
    this->nclients++;    
    if (this->nclients > 1) always_on = 1;
    pthread_mutex_unlock(&this->mutex);    
    /* Send the appropriate header */
    send_fileheader(sid, 0, 200, "OK", "1", "multipart/x-mixed-replace;boundary=BOUNDARY", -1, now);
    prints("--BOUNDARY\r\n");
    flushbuffer(sid);
    /* if always on or clients already listening then there is already a frame */    
    if (always_on) {
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

/** Internal function to alloc a correctly sized processed array */
void ffmpegStream::allocScArray(size_t size) {
    if (this->scArray) {
        if (this->scArray->dims[0].size >= size) {
            /* the processed array is already big enough */
            av_image_fill_arrays(scPicture->data,scPicture->linesize,(uint8_t *)scArray->pData,c->pix_fmt,c->width,c->height,1);
            return;
        } else {
            /* need a new one, so discard the old one */
            this->scArray->release();
        }
    }
    this->scArray = this->pNDArrayPool->alloc(1, &size, NDInt8, 0, NULL);
    /* alloc in and scaled pictures */
    av_image_fill_arrays(scPicture->data,scPicture->linesize,(uint8_t *)scArray->pData,c->pix_fmt,c->width,c->height,1);
}       
    

/** Take an NDArray, add grid and false colour, compress it to a jpeg, then
 * signal to the server process that there is a new frame available.
 */
void ffmpegStream::processCallbacks(NDArray *pArray)
{
//    double difftime;
//    struct timeval start, end;
//    gettimeofday(&start, NULL);     
    /* we're going to get these with getIntegerParam */
    int quality, clients, false_col, always_on, maxw, maxh;
    /* we're going to get these from the dims of the image */
    int width, height;
    /* in case we force a final size */
    int setw, seth;
    int num_bytes = 0;
    AVPacket *pkt = NULL;
    NDArray *pScratch = NULL;
    int ret;
	char errbuf[AV_ERROR_MAX_STRING_SIZE];

    size_t size;
    /* for printing errors */
    const char *functionName = "processCallbacks";
    /* for getting the colour mode */
    int colorMode = NDColorModeMono;
    NDAttribute *pAttribute = NULL;    

    /* Call the base class method */
    NDPluginDriver::beginProcessCallbacks(pArray);
    
    /* see if anyone's listening */
    pthread_mutex_lock(&this->mutex);    
    clients = this->nclients;
    pthread_mutex_unlock(&this->mutex);  
    setIntegerParam(0, ffmpegServerClients, clients);
    
    /* get the configuration values */
    getIntegerParam(0, ffmpegServerQuality, &quality);
    getIntegerParam(0, ffmpegServerFalseCol, &false_col);
    getIntegerParam(0, ffmpegServerAlwaysOn, &always_on);
    getIntegerParam(0, ffmpegServerMaxW, &maxw);
    getIntegerParam(0, ffmpegServerMaxH, &maxh);
    getIntegerParam(0, ffmpegServerSetW, &setw);
    getIntegerParam(0, ffmpegServerSetH, &seth);

    /* if no-ones listening and we're not always on then do nothing */
    if (clients == 0 && always_on == 0) {
//        printf("No-one listening\n");
        return;
    }

    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing memory
     * that other threads can access. */
    this->unlock();
     
    /* Get the colormode of the array */    
    pAttribute = pArray->pAttributeList->find("ColorMode");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colorMode);
    if ((pArray->ndims == 2) && (colorMode == NDColorModeMono)) {
        width  = (int) pArray->dims[0].size;
        height = (int) pArray->dims[1].size;
    } else if ((pArray->ndims == 3) && (pArray->dims[0].size == 3) && (colorMode == NDColorModeRGB1)) {
        width  = (int) pArray->dims[1].size;
        height = (int) pArray->dims[2].size;
    } else {
        width  = (int) pArray->dims[0].size;
        height = (int) pArray->dims[1].size;
    }
    /* scale image according to user request */
    if (setw > 0 && seth > 0) {
        width = setw;
        height = seth;
    } else if (setw > 0) {
        double sf = (double)(setw)/width;
        height = (int)(sf * height);
        width = setw;
    } else if (seth > 0) {
        double sf = (double)(seth)/height;
        width = (int)(sf * width);
        height = seth;
    }

    /* If we exceed the maximum size */
    if (width > maxw || height > maxh) {
    	double sf = MIN(((double)maxw)/width, ((double)maxh)/height);
    	width = (int) (sf * width);
    	height = (int) (sf * height);
    }

    /* If width and height have changed then reinitialise the codec */
    if (c == NULL || width != c->width || height != c->height) {
//        printf("Setting width %d height %d\n", width, height);
        AVRational avr;
        avr.num = 1;
        avr.den = 25;
        if (c != NULL) {
            /* width and height changed, close old codec */
            avcodec_close(c);
            av_free(c);
        }
        c = avcodec_alloc_context3(codec);
        /* Make sure that we don't try and create an image smaller than AV_INPUT_BUFFER_MIN_SIZE */
        if (width * height < AV_INPUT_BUFFER_MIN_SIZE) {
        	double sf = sqrt(1.0 * AV_INPUT_BUFFER_MIN_SIZE / width / height);
        	height = (int) (height * sf + 1);
        	if (height % 32) height = height + 32 - (height % 32);
        	width = (int) (width * sf + 1);        	
        	if (width % 32) width = width + 32 - (width % 32);
		}        	
        c->width = width;
        c->height = height;
        c->flags = AV_CODEC_FLAG_QSCALE;
        c->time_base = avr;
        c->pix_fmt = AV_PIX_FMT_YUVJ420P;
        if(codec && codec->pix_fmts){
            const enum AVPixelFormat *p= codec->pix_fmts;
            for(; *p!=-1; p++){
                if(*p == c->pix_fmt)
                    break;
            }
            if(*p == -1)
                c->pix_fmt = codec->pix_fmts[0];
        }
        /* open it */
        if (avcodec_open2(c, codec, NULL) < 0) {
            c = NULL;
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s: could not open codec\n",
                driverName, functionName);
            return;
        }
        /* Override codec pix_fmt to get rid of error messages */
        if (c->pix_fmt == AV_PIX_FMT_YUVJ420P) {
        	c->pix_fmt = AV_PIX_FMT_YUV420P;
        	c->color_range = AVCOL_RANGE_JPEG;
    	}
    }
    size = width *  height;
    
    /* make sure our processed array is big enough */
    this->allocScArray(2 * size);

    /* Set the quality */    
    scPicture->quality = 3276 - (int) (quality * 32.76);
    if (scPicture->quality < 0) scPicture->quality = 0;
    if (scPicture->quality > 32767) scPicture->quality = 32768;    
    
    /* format the array */
    if (formatArray(pArray, this->pasynUserSelf, this->inPicture,
        &(this->ctx), this->c, this->scPicture) != asynSuccess) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: Could not format array for correct pix_fmt for codec\n",
            driverName, functionName);    
        /* We must enter the loop and exit with the mutex locked */
        this->lock();                
        return;
    }      

    /* lock the output plugin mutex */
    pthread_mutex_lock(&this->mutex);

    /* send the frame to the encoder */
    ret = avcodec_send_frame(c, scPicture);
    if (ret < 0) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s Error sending a frame to the encoder: %s\n",
            driverName, functionName, av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret));
        goto done;
    }

    /* allocate a new NDArray for encoded packet */
    pScratch = this->pNDArrayPool->alloc(1, &size, NDInt8, 0, NULL);

    pkt = av_packet_alloc();
    while (ret >= 0) {
        ret = avcodec_receive_packet(c, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s Error encoding a frame: %s\n",
                driverName, functionName, av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret));
            pScratch->release();
            goto done;
        }
        memcpy((char*)pScratch->pData + num_bytes, pkt->data, pkt->size);
        num_bytes += pkt->size;

        av_packet_unref(pkt);
    }
    pScratch->dims[0].size = num_bytes;

    /* Release the last jpeg created */
    if (this->jpeg) {
        this->jpeg->release();
    }
    this->jpeg = pScratch;

    //printf("Frame! Size: %d\n", this->jpeg->dims[0].size);
    
    /* signal fresh_frame to output plugin and unlock mutex */
    for (int i=0; i<config.server_maxconn; i++) {
        pthread_cond_signal(&(this->cond[i]));
    }

done:
    av_packet_free(&pkt);
    pthread_mutex_unlock(&this->mutex);

    /* We must enter the loop and exit with the mutex locked */
    this->lock();

    /* Update the parameters.  */
    callParamCallbacks(0, 0);
//    gettimeofday(&end, NULL);   
//    difftime = (end.tv_usec - start.tv_usec) * 0.000001 + end.tv_sec - start.tv_sec;
//    if (difftime > 0.1) printf ("It took %.2lf seconds to process callbacks. That's a bit slow\n", difftime);      
    
}

/** Constructor for ffmpegStream; Class representing an mjpg stream served up by ffmpegServer. 
ffmpegServerConfigure() must be called before creating any instances of this 
class. ffmpegStreamConfigure() should be used to create an instance in the iocsh.
See ffmpegStream.template for more details of usage.

  * \param portName The name of the asyn port driver to be created.
  * \param queueSize The number of NDArrays that the input queue for this plugin can hold when 
  *        NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *        at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *        0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *        of the driver doing the callbacks.
  * \param NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is 
  *        allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *        allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
 */
ffmpegStream::ffmpegStream(const char *portName, int queueSize, int blockingCallbacks,
                           const char *NDArrayPort, int NDArrayAddr, int maxBuffers, int maxMemory,
                           int priority, int stackSize)
    /* Invoke the base class constructor
     * Set autoconnect to 1.  priority can be 0, which will use defaults. 
     * We require a minimum stacksize of 128k for windows-x64 */    
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory,
                   asynGenericPointerMask,
                   asynGenericPointerMask,
                   0, 1, priority, stackSize < 128000 ? 128000 : stackSize, 1)  /* Not ASYN_CANBLOCK or ASYN_MULTIDEVICE, do autoConnect */
{
    char host[64] = "";
    char url[256] = "";
    asynStatus status;
    this->jpeg = NULL;
    this->scArray = NULL;
    this->nclients = 0;
    this->c = NULL;
    this->codec = NULL;         
    this->inPicture = NULL;
    this->scPicture = NULL;            
    this->ctx = NULL;      
    this->cond = NULL;

    /* Create some parameters */
    createParam(ffmpegServerQualityString,  asynParamInt32, &ffmpegServerQuality);
    createParam(ffmpegServerFalseColString, asynParamInt32, &ffmpegServerFalseCol);
    createParam(ffmpegServerHttpPortString, asynParamInt32, &ffmpegServerHttpPort);
    createParam(ffmpegServerHostString,     asynParamOctet, &ffmpegServerHost);
    createParam(ffmpegServerJpgUrlString,   asynParamOctet, &ffmpegServerJpgUrl);
    createParam(ffmpegServerMjpgUrlString,  asynParamOctet, &ffmpegServerMjpgUrl);
    createParam(ffmpegServerClientsString,  asynParamInt32, &ffmpegServerClients);
    createParam(ffmpegServerAlwaysOnString, asynParamInt32, &ffmpegServerAlwaysOn);
    createParam(ffmpegServerMaxWString,     asynParamInt32, &ffmpegServerMaxW);
    createParam(ffmpegServerMaxHString,     asynParamInt32, &ffmpegServerMaxH);
    createParam(ffmpegServerSetWString,     asynParamInt32, &ffmpegServerSetW);
    createParam(ffmpegServerSetHString,     asynParamInt32, &ffmpegServerSetH);

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
    sprintf(url, "http://%s:%d/%s.jpg", host, config.server_port, portName);
    setStringParam(ffmpegServerJpgUrl, url);
    sprintf(url, "http://%s:%d/%s.mjpg", host, config.server_port, portName);
    setStringParam(ffmpegServerMjpgUrl, url);

    /* Initialise the ffmpeg library */
    ffmpegInitialise();

    /* make the input and output pictures */
    inPicture = av_frame_alloc();
    scPicture = av_frame_alloc();

    /* Setup correct codec for mjpeg */
    codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!codec) {
        fprintf(stderr, "MJPG codec not found\n");
        exit(1);
    }
    
    /* this mutex and the conditional variable are used to synchronize access to the global picture buffer */
    pthread_mutex_init(&(this->mutex), NULL);
    this->cond = (pthread_cond_t *) calloc(config.server_maxconn, sizeof(pthread_cond_t));
    for (int i=0; i<config.server_maxconn; i++) {
        pthread_cond_init(&(this->cond[i]), NULL);
    }
}

/** Configuration routine.  Called directly, or from the iocsh function, calls ffmpegStream constructor:

\copydoc ffmpegStream::ffmpegStream
*/
extern "C" int ffmpegStreamConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                  const char *NDArrayPort, int NDArrayAddr, int maxBuffers, int maxMemory,
                                  int priority, int stackSize)
{
    if (nstreams+1 > MAX_FFMPEG_STREAMS) {
        printf("%s:ffmpegStreamConfigure: Can only create %d streams\n",
            driverName, MAX_FFMPEG_STREAMS);
        return(asynError);        
    }
    ffmpegStream *pPlugin = new ffmpegStream(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxBuffers, maxMemory, priority, stackSize);
    pPlugin->start();
    streams[nstreams++] = pPlugin;
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
static const iocshArg serverArg1 = { "Network Interface", iocshArgString};
static const iocshArg * const serverArgs[] = {&serverArg0, &serverArg1};
static const iocshFuncDef serverFuncDef = {"ffmpegServerConfigure",2,serverArgs};


static void serverCallFunc(const iocshArgBuf *args)
{
    if(args[1].sval == NULL)
        ffmpegServerConfigure(args[0].ival, "any");
    else
        ffmpegServerConfigure(args[0].ival, args[1].sval);
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
/**
 * \file
 * section License
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
