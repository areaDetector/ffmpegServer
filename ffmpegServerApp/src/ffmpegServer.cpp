/* local includes */
#include "ffmpegServer.h"

/* EPICS includes */
#include "epicsExport.h"
#include "iocsh.h"
#include <epicsExit.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <time.h>
#include <math.h>

static const char *driverName = "ffmpegServer";

static MHD_Daemon *mhd_daemon = NULL;
static int mhd_port = 8080;
static int mhd_maxconn = 50;

static ffmpegStream **streams;
static int nstreams = 0;

class Streamer {

private:
    ffmpegStream *mStream;
    char *mBuffer;
    size_t mSent;
    size_t mRemaining;

public:
    Streamer(ffmpegStream *stream)
    : mStream(stream), mBuffer(NULL), mSent(0), mRemaining(0)
    {
    }

    ~Streamer(void) {
        clear_frame();
        mStream->send_stream_done();
    }

    void clear_frame(void) {
        free(mBuffer);
        mBuffer = NULL;
        mSent = mRemaining = 0;
    }

    ssize_t get_frame(void)
    {
        NDArray *pArray = mStream->get_jpeg(true);

        if (!pArray)
            return 0;

        size_t data_len = pArray->dims[0].size;
        mBuffer = (char*) malloc(data_len + 2048);

        size_t length = 0;

        #define APPEND(content)\
        do {\
            const char *c = content;\
            size_t c_len = strlen(c);\
            memcpy(mBuffer + length, c, c_len);\
            length += c_len;\
        } while(0)

        // Header
        APPEND("BOUNDARY\r\n");
        APPEND(MHD_HTTP_HEADER_CONTENT_TYPE ": image/jpeg\r\n\r\n");
        #undef APPEND

        // Data
        memcpy(mBuffer + length, pArray->pData, data_len);
        pArray->release();
        length += data_len;

        mRemaining = length;
        mSent = 0;

        return length;
    }

    ssize_t send_frame(uint64_t pos, char *buf, size_t max)
    {
        if (!mRemaining) {
            clear_frame();
            get_frame();
        }

        size_t length = std::min(mRemaining, max);
        memcpy(buf, mBuffer + mSent, length);

        mSent += length;
        mRemaining -= length;

        return length;
    }
};


char *get_index(void)
{
    const char *body_top =
        "<HTML> \n"
        " <style type=\"text/css\"> \n"
        "BODY { \n"
        "  background-color: #f2f2f6; \n"
        "  font-family: arial, sans-serif; \n"
        "} \n"
        "H1 { \n"
        "  font-size: 24px; \n"
        "  color: #000000; \n"
        "  font-weight: bold; \n"
        "  text-align: center; \n"
        "} \n"
        "H2 { \n"
        "  font-size: 18px; \n"
        "  color: #1b2a60; \n"
        "  font-weight: bold; \n"
        "  text-align: center; \n"
        "} \n"
        "A:link { \n"
        "  text-decoration: none; color:#3b4aa0; \n"
        "} \n"
        "A:visited { \n"
        "  text-decoration: none; color:#3b4aa0; \n"
        "} \n"
        "A:active { \n"
        "  text-decoration: none; color:#3b4aa0; \n"
        "} \n"
        "A:hover { \n"
        "  text-decoration: underline; color:#3b4aff; \n"
        "} \n"
        "td { \n"
        "  background-color: #e0e0ee; \n"
        "  border-style: outset; \n"
        "  border-color: #e0e0ee; \n"
        "  border-width: 1px; \n"
        "  padding: 10px; \n"
        "  text-align: center; \n"
        "} \n"
        "p { \n"
        "  text-align: center; \n"
        "} \n"
        " </style> \n"
        " <HEAD> \n"
        "  <TITLE>ffmpegServer Content Listing</TITLE> \n"
        " </HEAD> \n"
        " <BODY> \n"
        "  <CENTER> \n"
        "   <H1>ffmpegServer Content Listing</H1> \n"
        "   <TABLE cellspacing=\"15\"> \n"
        "    <TR> \n";

    const char *body_bottom =
        "</TR>\n"
        "</TABLE>\n"
        "</CENTER>\n"
        "</BODY>\n"
        "</HTML>\n";

    const char *stream_cell =
        "<TD>\n"
        "<H2>%s</H2>\n"
        "<img src=\"%s.index\" height=\"192\" title=\"Static JPEG\" alt=\"&lt;No image yet&gt;\"/>\n"
        "<p><a href=\"%s.jpg\">Link to Static JPEG</a></p>\n"
        "<p><a href=\"%s.mjpg\">Link to MJPEG Stream</a></p>\n"
        "</TD>\n";

    const char *new_line = "</TR><TR>\n";
    size_t new_line_len = strlen(new_line);

    size_t available = 131072;

    char *buffer = (char*)malloc(available);
    buffer[0] = '\0';

    size_t body_top_len = strlen(body_top);
    if (available - 1 < body_top_len)
        return buffer;

    strncat(buffer, body_top, available-1);
    available -= body_top_len;

    /* make a table of streams */
    for (int i=0; i<nstreams; i++) {
        char stream_buf[4096];
        snprintf(stream_buf, 4096, stream_cell, streams[i]->portName, streams[i]->portName,
                                                streams[i]->portName, streams[i]->portName);

        size_t stream_buf_len = strlen(stream_buf);
        if (available - 1 < stream_buf_len)
            return buffer;

        strncat(buffer, stream_buf, available-1);
        available -= stream_buf_len;

        if (i % 3 == 2) { //3 per row
            if (available - 1 < new_line_len)
                return buffer;

            strncat(buffer, new_line, available-1);
            available -= new_line_len;
        }
    }

    size_t body_bottom_len = strlen(body_bottom);
    if (available - 1 < body_bottom_len)
        return buffer;

    strncat(buffer, body_bottom, available-1);

    return buffer;
}

/** This is called whenever a client requests a stream */
MHD_Result dorequest(void *cls, struct MHD_Connection *connection, const char *url,
              const char *method, const char *version, const char *upload_data,
              size_t *upload_data_size, void **con_cls)
{
    if (strcmp(method, MHD_HTTP_METHOD_GET))
        return MHD_NO;

    /* check if we are asking for a jpg or an mjpg file */
    const char *ext = strrchr(url, '.');

    if (!ext) {
        char *index_buf = get_index();

        struct MHD_Response *response = MHD_create_response_from_buffer(
                       strlen(index_buf),
                       (void*) index_buf,
                       MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");

        MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    int len = (int) (ext - url - 1);
    char portName[256] = {0};
    strncpy(portName, url+1, len);
    ext++;
    for (int i=0; i<nstreams; i++) {
        if (strcmp(portName, streams[i]->portName) == 0) {
            if (strcmp(ext, "index") == 0)
                return (MHD_Result)streams[i]->send_snapshot(connection, 1);

            if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0)
                return (MHD_Result)streams[i]->send_snapshot(connection, 0);

            if (strcmp(ext, "mjpg") == 0 || strcmp(ext, "mjpeg") == 0)
                return (MHD_Result)streams[i]->send_stream(connection);
        }
    }
    return MHD_NO;
}

ssize_t send_frame(void *cls, uint64_t pos, char *buf, size_t max)
{
    return ((Streamer *)cls)->send_frame(pos, buf, max);
}

void send_stream_done(void *cls)
{
    Streamer *s = (Streamer*)cls;
    delete s;
}

static int stopping = 0;

/** c function that will be called at epicsExit that shuts down the http server cleanly */
void c_shutdown(void *) {
    printf("Shutting down http server...");
    stopping = 1;
    MHD_stop_daemon(mhd_daemon);
    sleep(1);
    printf("OK\n");
}    


/** Configure and start the http server, specifying a specific network interface, or 'any' for default.
 * To specify an interface, use either the DNS name or the IP of the NIC. ex: 10.68.212.33 or my-ioc-server-hostname
 * Default port is 8080. 
 */
void ffmpegServerConfigure(int port, const char* networkInterface) {
    if(port)
        mhd_port = port;

    streams = (ffmpegStream **) calloc(MAX_FFMPEG_STREAMS, sizeof(ffmpegStream *));
    nstreams = 0;
    printf("Starting server on port %d...\n", mhd_port);
    mhd_daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY |
                                   MHD_USE_THREAD_PER_CONNECTION,
                                   mhd_port, NULL,
                                   NULL, dorequest, NULL,
                                   MHD_OPTION_CONNECTION_LIMIT, mhd_maxconn,
                                   MHD_OPTION_CONNECTION_TIMEOUT, 120,
                                   MHD_OPTION_END);

#ifdef WIN32
    if (_beginthread(WSAReaper, 0, NULL)==-1) {
        logerror("Winsock reaper thread failed to start");
        exit(0);
    }
#endif
    printf("OK\n");

    /* Register the shutdown function for epicsAtExit */
    epicsAtExit(c_shutdown, NULL);    
}

/** Internal function to send a single snapshot */
void ffmpegStream::send_snapshot(MHD_Connection *connection, int index) {
    int always_on;
    NDArray* pArray;
//  printf("JPEG requested\n");
    /* Say we're listening */
    getIntegerParam(0, ffmpegServerAlwaysOn, &always_on);
    this->mutex.lock();
    this->nclients++;   
    if (this->nclients > 1) always_on = 1;
    this->mutex.unlock();
    /* if always on or clients already listening then there is already a frame */        
    if (always_on || index) {
        pArray = get_jpeg(false);
    } else {
        pArray = get_jpeg(true);
    }    
    /* we're no longer listening */
    this->mutex.lock();
    this->nclients--;
    this->mutex.unlock();
    /* If there's no data yet, say so */
    if (pArray == NULL) {
        char *nodata = (char*)malloc(1024);
        strcpy(nodata, "<html><title>No Data</title><body>No jpeg available yet</body></html>");
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(nodata), nodata, MHD_RESPMEM_MUST_FREE);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    /* Send the right header for a jpeg */
    int size = (int) pArray->dims[0].size;

    struct MHD_Response *response = MHD_create_response_from_buffer(
        size, (char *) pArray->pData, MHD_RESPMEM_PERSISTENT);

    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "image/jpeg");

    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    pArray->release();
    return ret;
}

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

/** Internal function to get the current jpeg and return it */
NDArray* ffmpegStream::get_jpeg(bool wait) {
    if (wait) {
        epicsEvent evt;
        epicsEvent *ptr = &evt;
        this->waiting.send(&ptr, sizeof(ptr));
        evt.wait();
     }
     NDArray* pArray;
     this->mutex.lock();
     pArray = this->jpeg;
     if(pArray) pArray->reserve();
     this->mutex.unlock();
     return pArray;
}

/** Internal function to send an mjpg stream */
void ffmpegStream::send_stream(MHD_Connection *connection) {
    int ret, always_on;
    /* Say we're listening */
    getIntegerParam(0, ffmpegServerAlwaysOn, &always_on);
    this->mutex.lock();
    this->nclients++;
    if (this->nclients > 1) always_on = 1;
    this->mutex.unlock();

    /* Create a new streamer object */
    Streamer *s = new Streamer(this);

    /* Send the appropriate header */
    struct MHD_Response *response = MHD_create_response_from_callback(
        MHD_SIZE_UNKNOWN, 16*1024*1024, &::send_frame, s,
        &::send_stream_done);

    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE,
        "multipart/x-mixed-replace; boundary=BOUNDARY");
    MHD_add_response_header(response, MHD_HTTP_HEADER_EXPIRES, "0");
    MHD_add_response_header(response, MHD_HTTP_HEADER_CACHE_CONTROL,
        "no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0");
    MHD_add_response_header(response, MHD_HTTP_HEADER_PRAGMA, "no-cache");

    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

void ffmpegStream::send_stream_done(void)
{
     /* We're no longer listening */
    this->mutex.lock();
    this->nclients--;    
    this->mutex.unlock();
} 

/** Internal function to alloc a correctly sized processed array */
void ffmpegStream::allocScArray(size_t size) {
    if (this->scArray) {
        if (this->scArray->dims[0].size >= size) {
            /* the processed array is already big enough */
            avpicture_fill((AVPicture *)scPicture,(uint8_t *)scArray->pData,c->pix_fmt,c->width,c->height);               
            return;
        } else {
            /* need a new one, so discard the old one */
            this->scArray->release();
        }
    }
    this->scArray = this->pNDArrayPool->alloc(1, &size, NDInt8, 0, NULL);
    /* alloc in and scaled pictures */
    avpicture_fill((AVPicture *)scPicture,(uint8_t *)scArray->pData,c->pix_fmt,c->width,c->height);   
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

    size_t size;
    /* for printing errors */
    const char *functionName = "processCallbacks";
    /* for getting the colour mode */
    int colorMode = NDColorModeMono;
    NDAttribute *pAttribute = NULL;    

    /* Call the base class method */
    NDPluginDriver::beginProcessCallbacks(pArray);
    
    /* see if anyone's listening */
    this->mutex.lock();
    clients = this->nclients;
    this->mutex.unlock();
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
    this->mutex.lock();

    /* Release the last jpeg created */
    if (this->jpeg) {
        this->jpeg->release();
    }
    
    /* Convert it to a jpeg */        
    this->jpeg = this->pNDArrayPool->alloc(1, &size, NDInt8, 0, NULL);

    AVPacket pkt;
    int got_output;
    av_init_packet(&pkt);
    pkt.data = (uint8_t*)this->jpeg->pData;    // packet data will be allocated by the encoder
    pkt.size = c->width * c->height;

    // needed to stop a stream of "AVFrame.format is not set" etc. messages
    scPicture->format = c->pix_fmt;
    scPicture->width = c->width;
    scPicture->height = c->height;

    if (avcodec_encode_video2(c, &pkt, scPicture, &got_output)) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: Encoding jpeg failed\n",
            driverName, functionName);
        got_output = 0; // got_output is undefined on error, so explicitly set it for use later
    }

    if (got_output) {
        this->jpeg->dims[0].size = pkt.size;
        av_packet_unref(&pkt);
    }

    //printf("Frame! Size: %d\n", this->jpeg->dims[0].size);
    
    /* signal fresh_frame to output plugin and unlock mutex */
    epicsEvent *evt;
    while(this->waiting.tryReceive(&evt, sizeof(evt)) != -1)
        evt->signal();

    this->mutex.unlock();

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
                   0, 1, priority, stackSize < 128000 ? 128000 : stackSize, 1),  /* Not ASYN_CANBLOCK or ASYN_MULTIDEVICE, do autoConnect */
    waiting(mhd_maxconn, sizeof(epicsEvent *))
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
    setIntegerParam(0, ffmpegServerHttpPort, mhd_port);
    setIntegerParam(0, ffmpegServerClients, 0);    
    
    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "ffmpegServer");    

    /* Set the hostname */
    gethostname(host, 64);
    setStringParam(ffmpegServerHost, host);   
    sprintf(url, "http://%s:%d/%s.jpg", host, mhd_port, portName);
    setStringParam(ffmpegServerJpgUrl, url);
    sprintf(url, "http://%s:%d/%s.mjpg", host, mhd_port, portName);
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
