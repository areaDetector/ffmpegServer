#include <QtDebug>
#include <QToolTip>
#include "ffmpegWidget.h"
#include <QColorDialog>
#include "colorMaps.h"
#include <QX11Info>
#include <assert.h>
#include <QImage>
#include <QPainter>

/* global switch for fallback mode */
int fallback = 0;

/* set this when the ffmpeg lib is initialised */
static int ffinit=0;

/* need this to protect certain ffmpeg functions */
static QMutex *ffmutex;

// An FFBuffer contains an AVFrame, a mutex for access and some data
FFBuffer::FFBuffer() {
    this->mutex = new QMutex();
    this->refs = 0;
    this->pFrame = avcodec_alloc_frame();
    this->mem = (unsigned char *) calloc(MAXWIDTH*MAXHEIGHT*3, sizeof(unsigned char));
}

FFBuffer::~FFBuffer() {
    av_free(this->pFrame);
    free(this->mem);
}

bool FFBuffer::grabFree() {
    if (this->mutex->tryLock()) {
        if (this->refs == 0) {
            this->refs += 1;
            this->mutex->unlock();    
            return true;
        } else {
            this->mutex->unlock();    
            return false;
        }
    } else {
        return false;
    }
}

void FFBuffer::reserve() {
    this->mutex->lock();    
    this->refs += 1;
    this->mutex->unlock();    
}

void FFBuffer::release() {
    this->mutex->lock();
    this->refs -= 1;
    this->mutex->unlock();    
}

// List of FFBuffers to use for raw frames
static FFBuffer rawbuffers[NBUFFERS];

// List of FFBuffers to use for uncompressed frames
static FFBuffer outbuffers[NBUFFERS];

// find a free FFBuffer
FFBuffer * findFreeBuffer(FFBuffer* source) {
    for (int i = 0; i < NBUFFERS; i++) {
        // if we can lock it and it has a 0 refcount, we can use it!
        if (source[i].mutex->tryLock()) {
            if (source[i].refs == 0) {
                source[i].refs += 1;
                source[i].mutex->unlock();    
                return &source[i];
            } else {
                source[i].mutex->unlock();    
            }
        }
    }
    return NULL;
}

/* thread that decodes frames from video stream and emits updateSignal when
 * each new frame is available
 */
FFThread::FFThread (const QString &url, QWidget* parent)
    : QThread (parent)
{
    // this is the url to read the stream from
    strcpy(this->url, url.toAscii().data());
    // set this to 1 to finish
    this->stopping = 0;
    // initialise the ffmpeg library once only
    if (ffinit==0) {
        ffinit = 1;
        // init mutext
        ffmutex = new QMutex();
        // only display errors
        av_log_set_level(AV_LOG_ERROR);
        // Register all formats and codecs
        av_register_all();
    }
}

// destroy widget
FFThread::~FFThread() {
}

// run the FFThread
void FFThread::run()
{
    AVFormatContext     *pFormatCtx;
    int                 videoStream;
    AVCodecContext      *pCodecCtx;
    AVCodec             *pCodec;
    AVPacket            packet;
    int                 frameFinished, len;

    // Open video file
    if (av_open_input_file(&pFormatCtx, this->url, NULL, 0, NULL)!=0) {
        printf("Opening input '%s' failed\n", this->url);
        return;
    }

    // Find the first video stream
    videoStream=-1;
    for (unsigned int i=0; i<pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    }
    if( videoStream==-1) {
        printf("Finding video stream in '%s' failed\n", this->url);
        return;
    }

    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        printf("Could not find decoder for '%s'\n", this->url);
        return;
    }

    // Open codec
    ffmutex->lock();
    if(avcodec_open(pCodecCtx, pCodec)<0) {
        printf("Could not open codec for '%s'\n", this->url);
        return;
    }
    ffmutex->unlock();

    // read frames into the packets
    while (stopping !=1 && av_read_frame(pFormatCtx, &packet) >= 0) {

        // Is this a packet from the video stream?
        if (packet.stream_index!=videoStream) {
            // Free the packet if not
            printf("Non video packet. Shouldn't see this...\n");
            av_free_packet(&packet);
            continue;
        }

        // grab a buffer to decode into
        FFBuffer *raw = findFreeBuffer(rawbuffers);        
        if (raw == NULL) {
            printf("Couldn't get a free buffer, skipping packet\n");
            av_free_packet(&packet);
            continue;
        }
        
        // Tell the codec to use this bit of memory
        pCodecCtx->internal_buffer = raw->mem;

        // Decode video frame
        len = avcodec_decode_video2(pCodecCtx, raw->pFrame, &frameFinished,
            &packet);
        if (!frameFinished) {
            printf("Frame not finished. Shouldn't see this...\n");
            av_free_packet(&packet);
            raw->release();
            continue;
        }
        
        // Set the internal buffer back to null so that we don't accidentally free it
        pCodecCtx->internal_buffer = NULL;
        
        // Fill in the output buffer
        raw->pix_fmt = pCodecCtx->pix_fmt;         
        raw->height = pCodecCtx->height;
        raw->width = pCodecCtx->width;                

        // Emit and free
        emit updateSignal(raw);
        av_free_packet(&packet);
    }
    // tidy up
    ffmutex->lock();
    avcodec_close(pCodecCtx);
    av_close_input_file(pFormatCtx);
    pCodecCtx = NULL;
    pFormatCtx = NULL;
    ffmutex->unlock();
}

ffmpegWidget::ffmpegWidget (QWidget* parent)
    : QWidget (parent)
{
    // xv stuff
    this->xv_port = -1;
    this->xv_format = -1;
    this->xv_image = NULL;
    this->dpy = NULL;
    this->maxW = 0;
    this->maxH = 0;
    this->setMinimumSize(64,48);
    /* Now setup QImage or xv whichever we have */
    this->ff_fmt = PIX_FMT_RGB24;
    if (fallback == 0) this->xvSetup();
    /* setup some defaults, we'll overwrite them with sensible numbers later */
    /* Private variables, read/write */
    _x = 0;             // x offset in image pixels
    _y = 0;             // y offset in image pixels
    _zoom = 30;         // zoom level
    _gx = 0;            // grid x in image pixels
    _gy = 0;            // grid y in image pixels
    _gs = 0;            // grid spacing in image pixels    
    _grid = false;      // grid on or off
    _gcol = Qt::white;  // grid colour
    _fcol = 0;          // false colour
    _url = QString(""); // ffmpeg url
    this->disableUpdates = false;
    /* Private variables: read only */
    _maxX = 0;    // Max x offset in image pixels
    _maxY = 0;    // Max y offset in image pixels
    _maxGx = 0;   // Max grid x offset in image pixels
    _maxGy = 0;   // Max grid y offset in image pixels
    _imW = 0;     // Image width in image pixels
    _imH = 0;     // Image height in image pixels
    _visW = 0;    // Image width currently visible in image pixels
    _visH = 0;    // Image height currently visible in image pixels
    _scImW = 0;   // Image width in viewport scaled pixels
    _scImH = 0;   // Image height in viewport scaled pixels
    _scVisW = 0;  // Image width visible in viewport scaled pixels
    _scVisH = 0;  // Image height visible in viewport scaled pixels
    _fps = 0.0;   // Frames per second displayed
    // other
    this->sfx = 1.0;
    this->sfy = 1.0;    
    this->rawbuf = NULL;
    this->fullbuf = NULL;
    this->lastFrameTime = new QTime();
    this->ff = NULL;
    this->widgetW = 0;
    this->widgetH = 0;
    this->ctx = NULL;    
    // fps calculation
    this->tickindex = 0;
    this->ticksum = 0;
    for (int i=0; i<MAXTICKS; i++) {
        this->ticklist[i] = 0;
    }
    this->timer = new QTimer(this);
    connect(this->timer, SIGNAL(timeout()), this, SLOT(calcFps()));
    this->timer->start(100);
}

// destroy widget
ffmpegWidget::~ffmpegWidget() {
    ffQuit();
}

// setup x or xvideo
void ffmpegWidget::xvSetup() {
    XvAdaptorInfo * ainfo;
    XvEncodingInfo *encodings;
    unsigned int adaptors, nencode;
    unsigned int ver, rel, extmaj, extev, exterr;
    // get display number
    this->dpy = x11Info().display();
    // Grab the window id and setup a graphics context
    this->w = this->winId();
    this->gc = XCreateGC(this->dpy, this->w, 0, 0);
    // Now try and setup xv
    // return version and release of extension
    if (XvQueryExtension(this->dpy, &ver, &rel, &extmaj, &extev, &exterr) != Success) {
        printf("XvQueryExtension failed, using QImage fallback mode\n");
        return;
    }
    //printf("Ver: %d, Rel: %d, ExtMajor: %d, ExtEvent: %d, ExtError: %d\n", ver, rel, extmaj, extev, exterr);
    //Ver: 2, Rel: 2, ExtMajor: 140, ExtEvent: 75, ExtError: 150
    // return adaptor information for the screen
    if (XvQueryAdaptors(this->dpy, QX11Info::appRootWindow(),
            &adaptors, &ainfo) != Success) {
        printf("XvQueryAdaptors failed, using QImage fallback mode\n");
        return;
    }
    // see if we have any adapters
    if (adaptors <= 0) {
        printf("No xv adaptors found, using QImage fallback mode\n");
        return;
    }
    // grab the port from the first adaptor
    int gotPort = 0;
    for(int p = 0; p < (int) ainfo[0].num_ports; p++) {
        if(XvGrabPort(this->dpy, ainfo[0].base_id + p, CurrentTime) == Success) {
            this->xv_port = ainfo[0].base_id + p;
            gotPort = 1;
            break;
        }
    }
    // if we didn't find a port
    if (!gotPort) {
        printf("No xv ports free, using QImage fallback mode\n");
        return;
    }
    // get max XV Image size
    int gotEncodings = 0;
    XvQueryEncodings(this->dpy, ainfo[0].base_id, &nencode, &encodings);
    if (encodings && nencode && (ainfo[0].type & XvImageMask)) {
        for(unsigned int n = 0; n < nencode; n++) {
            if(!strcmp(encodings[n].name, "XV_IMAGE")) {
                this->maxW = encodings[n].width;
                this->maxH = encodings[n].height;
                gotEncodings = 1;
                break;
            }
        }
    }
    // if we didn't find a list of encodings
    if (!gotEncodings) {
        printf("No encodings information, using QImage fallback mode\n");
        return;
    }
    // only support I420 mode for now
    int num_formats = 0;
    XvImageFormatValues * vals = XvListImageFormats(this->dpy,
        this->xv_port, &num_formats);
    for (int i=0; i<num_formats; i++) {
        if (strcmp(vals->guid, "I420") == 0) {
            this->xv_format = vals->id;
            this->ff_fmt = PIX_FMT_YUVJ420P;
            // Widget is responsible for painting all its pixels with an opaque color
            setAttribute(Qt::WA_OpaquePaintEvent);
            setAttribute(Qt::WA_PaintOnScreen);
            return;
        }
        vals++;
    }
    printf("Display doesn't support I420 mode, using QImage fallback mode\n");
    return;
}

// take a buffer and swscale it to the requested dimensions
FFBuffer * ffmpegWidget::formatFrame(FFBuffer *src, PixelFormat pix_fmt) {
    FFBuffer *dest = findFreeBuffer(outbuffers);
    // make sure we got a buffer
    if (dest == NULL) return NULL;
    // fill in multiples of 8 that we can cope with
    dest->width = src->width - src->width % 8;
    dest->height = src->height - src->height % 2;
    dest->pix_fmt = pix_fmt;
    // see if we have a suitable cached context
    // note that we use the original values of width and height
    this->ctx = sws_getCachedContext(this->ctx,
        dest->width, dest->height, src->pix_fmt,
        dest->width, dest->height, dest->pix_fmt,
        SWS_BICUBIC, NULL, NULL, NULL);
    // Assign appropriate parts of buffer->mem to planes in buffer->pFrame    
    avpicture_fill((AVPicture *) dest->pFrame, dest->mem,
        dest->pix_fmt, dest->width, dest->height);
    // do the software scale
    sws_scale(this->ctx, src->pFrame->data, src->pFrame->linesize, 0,
        src->height, dest->pFrame->data, dest->pFrame->linesize);
    return dest;
}

// take a buffer and swscale it to the requested dimensions
FFBuffer * ffmpegWidget::falseFrame(FFBuffer *src, PixelFormat pix_fmt) {
    FFBuffer *yuv = NULL;
    switch (src->pix_fmt) {
        case PIX_FMT_YUV420P:   //< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
        case PIX_FMT_YUV411P:   //< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
        case PIX_FMT_YUVJ420P:  //< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV420P and setting color_range
        case PIX_FMT_NV12:      //< planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
        case PIX_FMT_NV21:      //< as above, but U and V bytes are swapped
        case PIX_FMT_YUV422P:   //< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
        case PIX_FMT_YUVJ422P:  //< planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV422P and setting color_range
        case PIX_FMT_YUV440P:   //< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
        case PIX_FMT_YUVJ440P:  //< planar YUV 4:4:0 full scale (JPEG), deprecated in favor of PIX_FMT_YUV440P and setting color_range
        case PIX_FMT_YUV444P:   //< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
        case PIX_FMT_YUVJ444P:  //< planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV444P and setting color_range
            yuv = src;
            break;
        default:
            yuv = formatFrame(src, PIX_FMT_YUVJ420P);
    }
    /* Now we have our YUV frame, generate YUV data */
    FFBuffer *dest = findFreeBuffer(outbuffers);
    // make sure we got a buffer
    if (dest == NULL) {
        // get rid of the original
        if (yuv != src) yuv->release();
        return NULL;
    }
    // fill in multiples of 8 that we can cope with
    dest->width = src->width - src->width % 8;
    dest->height = src->height - src->height % 2;
    dest->pix_fmt = pix_fmt;
    unsigned char *yuvdata = (unsigned char *) yuv->pFrame->data[0];
    unsigned char *destdata = (unsigned char *) dest->pFrame->data[0];
    if (pix_fmt == PIX_FMT_YUVJ420P) {
        const unsigned char * colorMapY, * colorMapU, * colorMapV;
        switch(_fcol) {
            case 2:
                colorMapY = IronColorY;
                colorMapU = IronColorU;
                colorMapV = IronColorV;
                break;
            default:
                colorMapY = RainbowColorY;
                colorMapU = RainbowColorU;
                colorMapV = RainbowColorV;
                break;
        }
        // Y planar data
        for (int h=0; h<dest->height; h++) {
            unsigned int line_start = yuv->pFrame->linesize[0] * h;
            for (int w=0; w<dest->width; w++) {
                *destdata++ = colorMapY[yuvdata[line_start + w]];
            }
        }
        // UV planar data
        for (int h=0; h<dest->height; h+=2) {
            unsigned int line_start = yuv->pFrame->linesize[0] * h;
            for (int w=0; w<dest->width; w+=2) {
                unsigned char y = yuvdata[line_start + w];
                destdata[h*dest->width/4 + w/2] = colorMapU[y];
                destdata[dest->height*dest->width/4 + h*dest->width/4 + w/2] = colorMapV[y];
            }
        }
    } else {
        // fill in RGB data
        const unsigned char * colorMapR, * colorMapG, * colorMapB;
        switch(this->_fcol) {
            case 2:
                colorMapR = IronColorR;
                colorMapG = IronColorG;
                colorMapB = IronColorB;
                break;
            default:
                colorMapR = RainbowColorR;
                colorMapG = RainbowColorG;
                colorMapB = RainbowColorB;
                break;
        }
        // RGB packed data
        for (int h=0; h<dest->height; h++) {
            unsigned int line_start = yuv->pFrame->linesize[0] * h;
            for (int w=0; w<dest->width; w++) {
                *destdata++ = colorMapR[yuvdata[line_start + w]];
                *destdata++ = colorMapG[yuvdata[line_start + w]];
                *destdata++ = colorMapB[yuvdata[line_start + w]];
            }
        }

    }
    // get rid of the original
    if (yuv != src) yuv->release();
    return dest;
}

void ffmpegWidget::updateImage(FFBuffer *newbuf) {
    // calculate fps
    int elapsed = this->lastFrameTime->elapsed();
    // limit framerate in fallback mode
    if (this->rawbuf && this->xv_format < 0 && elapsed < 100) {
        this->limited = QString(" (limited)");
        newbuf->release();
        return;
    }
    this->lastFrameTime->start();
    this->ticksum -= this->ticklist[tickindex];             /* subtract value falling off */
    this->ticksum += elapsed;                               /* add new value */
    this->ticklist[this->tickindex] = elapsed;              /* save new value so it can be subtracted later */
    if (++this->tickindex == MAXTICKS) this->tickindex=0;   /* inc buffer index */
    _fps = 1000.0  * MAXTICKS / this->ticksum;
    emit fpsChanged(_fps);
    emit fpsChanged(QString("%1%2").arg(_fps, 0, 'f', 1).arg(this->limited));
    this->limited = QString("");

    // store the buffer
    if (this->rawbuf) this->rawbuf->release();    
    this->rawbuf = newbuf;
    
    // make the frame the right format
    makeFullFrame();            

    // if width and height changes then make sure we zoom onto it
    if (this->fullbuf && (_imW != this->fullbuf->width || _imH != this->fullbuf->height)) {
        _imW = this->fullbuf->width;
        emit imWChanged(_imW);
        _imH = this->fullbuf->height;
        emit imHChanged(_imH);
        /* Zoom so it fills the viewport */
        disableUpdates = true;
        setZoom(0);
        /* Update the maximum gridx and gridy values */
        _maxGx = _imW - 1;
        emit maxGxChanged(_maxGx);
        _maxGy = _imH - 1;
        emit maxGyChanged(_maxGy);
        /* Update scale factors */
        this->updateScalefactor();
        this->disableUpdates = false;
    } 
    
    // repaint the screen
    update();
}

void ffmpegWidget::updateScalefactor() {
    /* Width or height of window has changed, so update scale */
    this->widgetW = width();
    this->widgetH = height();
    /* Work out which is the minimum ratio to scale by */
    double wratio = this->widgetW / (double) _imW;
    double hratio = this->widgetH / (double) _imH;
    double sf = pow(10, (double) (_zoom)/ 20) * qMin(wratio, hratio);
    /* Now work out the scaled dimensions */
    _scImW = (int) (_imW*sf + 0.5);
    emit scImWChanged(_scImW);
    _scImH = (int) (_imH*sf + 0.5);    
    emit scImHChanged(_scImH);
    _scVisW = qMin(_scImW, this->widgetW);
    emit scVisWChanged(_scVisW);
    _scVisH = qMin(_scImH, this->widgetH);
    emit scVisHChanged(_scVisH);     
    /* Now work out how much of the original image is visible */
    if (_scVisW < this->widgetW) {
        _visW = _imW;
    } else {
        _visW = qMin((int) (this->widgetW / sf + 0.5), _imW);
    }
    emit visWChanged(_visW);
    emit visWChanged(QString("%1").arg(_visW));
    if (_scVisH < this->widgetH) {
        _visH = _imH;
    } else {
        _visH = qMin((int) (this->widgetH / sf + 0.5), _imH);
    }
    emit visHChanged(_visH);
    emit visHChanged(QString("%1").arg(_visH));
    /* Now work out our real scale factors */
    this->sfx = _scVisW / (double) _visW;
    this->sfy = _scVisH / (double) _visH;      
    /* Now work out max x and y */;
    int maxX = qMax(_imW - _visW, 0);
    int maxY = qMax(_imH - _visH, 0);
    disableUpdates = true;
    if (_maxX != maxX) {
        _maxX = maxX;
        emit maxXChanged(_maxX);
        setX(qMin(_x, _maxX));
    }
    if (_maxY != maxY) {
        _maxY = maxY;
        emit maxYChanged(_maxY);
        setY(qMin(_y, _maxY));
    }
    disableUpdates = false;
    /* Now make an image */
    if (this->xv_format >= 0) {
        // xvideo supported
        if (this->xv_image) XFree(this->xv_image);
        this->xv_image = XvCreateImage(this->dpy, this->xv_port,
            this->xv_format, 0, _imW, _imH);
        assert(this->xv_image);
        /* Clear area not filled by image */
        if (_scVisW < this->widgetW) {
            XClearArea(dpy, w, _scVisW, 0, this->widgetW-_scVisW, this->widgetH, 0);
        }
        if (_scVisH < this->widgetH) {
            XClearArea(dpy, w, 0, _scVisH, this->widgetW, this->widgetH-_scVisH, 0);
        }
    }
}

void ffmpegWidget::makeFullFrame() {
    PixelFormat pix_fmt;    

    // make sure we have a raw buffer    
    if (this->rawbuf == NULL || this->rawbuf->width <= 0 || this->rawbuf->height <= 0) {
        return;
    }

    // if we have a raw buffer then decompress it
    this->rawbuf->reserve();

    // if we've got an image that's too big, force RGB
    if (this->ff_fmt == PIX_FMT_YUVJ420P && (this->rawbuf->width > maxW || this->rawbuf->height > maxH)) {
        printf("Image too big, using QImage fallback mode\n");
        pix_fmt = PIX_FMT_RGB24;
    } else {
        pix_fmt = this->ff_fmt;
    }

    // release any full frame we might have
    if (this->fullbuf) this->fullbuf->release();

    // Format the decoded frame as we've been asked        
    if (_fcol) {
        // make it false colour
        this->fullbuf = this->falseFrame(this->rawbuf, pix_fmt);
    } else {
        // pass out frame through sw_scale
        this->fullbuf = this->formatFrame(this->rawbuf, pix_fmt);
    }
    this->rawbuf->release();  

    // Check we got a buffer
    if (this->fullbuf == NULL) {
        printf("Couldn't get a free buffer, skipping frame\n");
        return;
    }    
      
    // draw the grid if asked to
#define overlayYPixel                 i = gsy * _imW + gsx; \
                yFrame[i] = (yFrame[i] * 4 + Y)/5
#define overlayUVPixel                 i = gsy * _imW/4 + gsx/2; \
                uFrame[i] = (uFrame[i] * 4 + U)/5; \
                vFrame[i] = (vFrame[i] * 4 + V)/5   

    // draw grid straight on image if xvideo
    if (_grid && this->xv_format >= 0 && _gs > 0) {
        unsigned char Y = (unsigned char) (0.299 * _gcol.red() + 0.587 * _gcol.green() + 0.114 * _gcol.blue());
        unsigned char U = (unsigned char) (-0.169 * _gcol.red() - 0.331 * _gcol.green() + 0.499 * _gcol.blue() + 128);
        unsigned char V = (unsigned char) (0.499 * _gcol.red() - 0.418 * _gcol.green() - 0.0813 * _gcol.blue() + 128);
        unsigned char *yFrame = this->fullbuf->pFrame->data[0];        
        unsigned char *uFrame = this->fullbuf->pFrame->data[0] + _imW * _imH;
        unsigned char *vFrame = this->fullbuf->pFrame->data[0] + _imW * _imH * 5 / 4; 
        int i;                  
        int gridw = 1;
        if (this->sfx > 0) gridw = qMax((int) (0.5 + 1 / this->sfx), 1);
        // X Lines           
        // Intensity data
        for (int gsy = 0; gsy < _imH; gsy += 1) {
            // X Minors
            for (int gsx = _gx - _gs; gsx > 0; gsx -= _gs) {
                overlayYPixel;
            }
            for (int gsx = _gx + _gs; gsx < _imW; gsx += _gs) {
                overlayYPixel;
            }
            // X Major
            for (int gsx = (int) (_gx + 0.5 - gridw/2.0); gsx < _gx - 0.1 + gridw/2.0; gsx++) {
                yFrame[gsy * _imW + gsx] = Y;            
            }
        }             
        // UV data
        for (int gsy = 0; gsy < _imH; gsy += 2) {
            // X Minors
            for (int gsx = _gx - _gs; gsx > 0; gsx -= _gs) {
                overlayUVPixel;
            }
            for (int gsx = _gx + _gs; gsx < _imW; gsx += _gs) {
                overlayUVPixel;
            }
            // X Major
            i = gsy * _imW/4 + _gx/2;            
            uFrame[i] = (uFrame[i] + U)/2;
            vFrame[i] = (vFrame[i] + V)/2;                                    
        }    
        // Y Lines        
        // Intensity data
        for (int gsx = 0; gsx < _imW; gsx += 1) {
            for (int gsy = _gy - _gs; gsy > 0; gsy -= _gs) {
                overlayYPixel;
            }
            for (int gsy = _gy + _gs; gsy < _imH; gsy += _gs) {
                overlayYPixel;
            }
            for (int gsy = (int) (_gy + 0.5 - gridw/2.0); gsy < _gy - 0.1 + gridw/2.0; gsy++) {
                yFrame[gsy * _imW + gsx] = Y;            
            }                    
        }             
        // UV data
        for (int gsx = 0; gsx < _imW; gsx += 2) {
            for (int gsy = _gy - _gs; gsy > 0; gsy -= _gs) {
                overlayUVPixel;
            }
            for (int gsy = _gy + _gs; gsy < _imH; gsy += _gs) {
                overlayUVPixel;
            }
            i = ((int)(_gy/2)) * _imW/2 + gsx/2;
            uFrame[i] = (uFrame[i] + U)/2;
            vFrame[i] = (vFrame[i] + V)/2;                                    
        }             
    }    
}

void ffmpegWidget::paintEvent(QPaintEvent *) {
    // check we have a full buffer
    if (this->fullbuf == NULL || this->fullbuf->width <= 0 || this->fullbuf->height <= 0) {
        return;
    }
    // If scale factors have changed
    if (this->widgetW != width() || this->widgetH != height()) {
        updateScalefactor();
    }
    FFBuffer * cachedFull = this->fullbuf;
    cachedFull->reserve();    
    if (cachedFull->pix_fmt == PIX_FMT_YUVJ420P) {
        // xvideo supported
        this->xv_image->data = (char *) cachedFull->pFrame->data[0];
        /* Draw the image */
        XvPutImage(this->dpy, this->xv_port, this->w, this->gc, this->xv_image,
            _x, _y, _visW, _visH, 0, 0, _scVisW, _scVisH);
   } else {
        // QImage fallback
        QPainter painter(this);
        QImage image(cachedFull->pFrame->data[0], cachedFull->width, cachedFull->height, QImage::Format_RGB888);
        painter.drawImage(QPoint(0, 0), image.copy(QRect(_x, _y, _visW, _visH)).scaled(_scVisW, _scVisH));
        /* Draw the grid */
        if (_grid) {
            QPainter painter(this);
            // note the 0.5 gives us the middle of the pixel
            double scGx = (_gx-_x+0.5)*this->sfx;
            double scGy = (_gy-_y+0.5)*this->sfy;
            double scGsx = _gs*this->sfx;
            double scGsy = _gs*this->sfy;        
            if (scGsx > 0.1 && scGsy > 0.1) {
                // Draw minor lines
                QColor gscol = QColor(_gcol);
                gscol.setAlpha(35);
                painter.setPen(gscol);
                // X lines to the left of crosshair
                for (double scx = scGx - scGsx; scx > 0; scx -= scGsx) {
                    painter.drawLine((int)(scx+0.5), 0, (int)(scx+0.5), _scVisH);
                }
                // X lines to the right of crosshair
                for (double scx = scGx + scGsx; scx < _scVisW; scx += scGsx) {
                    painter.drawLine((int)(scx+0.5), 0, (int)(scx+0.5), _scVisH);
                }
                // Y lines above the crosshair
                for (double scy = scGy - scGsy; scy > 0; scy -= scGsy) {
                    painter.drawLine(0, (int)(scy+0.5), _scVisW, (int)(scy+0.5));
                }
                // Y lines below the crosshair
                for (double scy = scGy + scGsy; scy < _scVisH; scy += scGsy) {
                    painter.drawLine(0, (int)(scy+0.5), _scVisW, (int)(scy+0.5));
                }
            }
            // Draw crosshairs
            painter.setPen(_gcol);
            painter.drawLine((int)(scGx+0.5), 0, (int)(scGx+0.5), _scVisH);
            painter.drawLine(0, (int)(scGy+0.5), _scVisW, (int)(scGy+0.5));
        }
    }
    cachedFull->release();
}

void ffmpegWidget::ffQuit() {
    // Tell the ff thread to stop
    if (ff==NULL) return;
    emit aboutToQuit();
    if (!ff->wait(500)) {
        // thread won't stop, kill it
        ff->terminate();
        ff->wait(100);
    }
    delete ff;
    ff = NULL;
}

// update grid centre
void ffmpegWidget::mouseDoubleClickEvent (QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && _grid) {
        disableUpdates = true;
        setGx((int) (_x + event->x()/this->sfx));
        disableUpdates = false;        
        setGy((int) (_y + event->y()/this->sfy));
        event->accept();
    }
}

// store the co-ordinates of the original mouse click and the old x and y
void ffmpegWidget::mousePressEvent (QMouseEvent* event) {
     if (event->button() == Qt::LeftButton) {
         clickx = event->x();
         oldx = _x;
         clicky = event->y();
         oldy = _y;
         event->accept();
     }
}

// drag the screen round so the pixel "grabbed" stays under the cursor
void ffmpegWidget::mouseMoveEvent (QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        // disable automatic updates
        disableUpdates = true;
        setX(oldx + (int)((clickx - event->x())/this->sfx));
        disableUpdates = false;        
        setY(oldy + (int)((clicky - event->y())/this->sfy));
        event->accept();
    }
}

// zoom in and out on the cursor
void ffmpegWidget::wheelEvent(QWheelEvent * event) {
    // disable automatic updates
    disableUpdates = true;
    // this is the fraction of screen dims that mouse is across
    double fx = event->x()/(double)_scVisW;
    double fy = event->y()/(double)_scVisH;
    int old_visW = _visW;
    int old_visH = _visH;
    if (event->delta() > 0) {
        this->setZoom(this->zoom() + 1);
    } else {
        this->setZoom(this->zoom() - 1);
    }
    // now work out where it is when zoom has changed
    setX(_x + (int) (0.5 + fx * (old_visW - _visW)));
    disableUpdates = false;    
    setY(_y + (int) (0.5 + fy * (old_visH - _visH)));
    event->accept();
}

// set the grid colour via a dialog
void ffmpegWidget::setGcol() {
    setGcol(QColorDialog::getColor(_gcol, this, QString("Choose grid Colour")));
}

// start or reset ffmpeg
void ffmpegWidget::setReset() {
    // must have a url
    if (_url=="") return;
    if ((ff!=NULL)&&(ff->isRunning())) {
        ffQuit();
    }

    // first make sure we don't update anything too quickly
    disableUpdates = true;

    /* create the ffmpeg thread */
    ff = new FFThread(_url, this);
    
    QObject::connect( ff, SIGNAL(updateSignal(FFBuffer *)),
                      this, SLOT(updateImage(FFBuffer *)) );
    QObject::connect( this, SIGNAL(aboutToQuit()),
                      ff, SLOT(stopGracefully()) );
    // allow updates, and start the image thread
    setZoom(0);    
    disableUpdates = false;
    ff->start();
}

// set fps to 0 if we've waited 1.5 times the time we should for a frame
void ffmpegWidget::calcFps() {
    if (this->lastFrameTime->elapsed() > 1500.0 / _fps) {
        emit fpsChanged(QString("0.0"));
    }
}

// x offset in image pixels
void ffmpegWidget::setX(int x) {
    x = x < 0 ? 0 : (x > _maxX) ? _maxX : x;
    // xvideo only accepts multiple of 2 offsets    
    if (this->xv_format >= 0) x = x - x % 2;    
    if (_x != x) {
        _x = x;
        emit xChanged(x);
        if (!disableUpdates) {
            update();
        }
    }
}

// y offset in image pixels
void ffmpegWidget::setY(int y) {
    y = y < 0 ? 0 : (y > _maxY) ? _maxY : y;
    // xvideo only accepts multiple of 2 offsets
    if (this->xv_format >= 0) y = y - y % 2;        
    if (_y != y) {
        _y = y;
        emit yChanged(y);
        if (!disableUpdates) {
            update();
        }
    }
}

// zoom level
void ffmpegWidget::setZoom(int zoom) {
    zoom = (zoom < 0) ? 0 : (zoom > 30) ? 30 : zoom;
    if (_zoom != zoom) {
        _zoom = zoom;
        emit zoomChanged(zoom);
        updateScalefactor();
        if (!disableUpdates) {
            update();
        }
    }
}

// grid x in image pixels
void ffmpegWidget::setGx(int gx) {
    gx = (gx < 1) ? 1 : (gx > _imW - 1 && _imW > 0) ? _imW - 1 : gx;
    if (_gx != gx) {
        _gx = gx;
        emit gxChanged(gx);
        if (!disableUpdates) {
            // Grid changed, so redraw it on current image
            if (this->xv_format >= 0) makeFullFrame();
            update();
        }
    }
}

// grid y in image pixels
void ffmpegWidget::setGy(int gy) {
    gy = (gy < 1) ? 1 : (gy > _imH - 1 && _imH > 0) ? _imH - 1 : gy;
    if (_gy != gy) {
        _gy = gy;
        emit gyChanged(gy);
        if (!disableUpdates) {
            // Grid changed, so redraw it on current image
            if (this->xv_format >= 0) makeFullFrame();
            update();
        }
    }
}

// grid spacing in image pixels
void ffmpegWidget::setGs(int gs) {
    gs = (gs < 10) ? 10 : (gs > 2000) ? 2000 : gs;
    if (_gs != gs) {
        _gs = gs;
        emit gsChanged(gs);
        if (!disableUpdates) {
            // Grid changed, so redraw it on current image
            if (this->xv_format >= 0) makeFullFrame();        
            update();
        }
    }
}

// grid on or off
void ffmpegWidget::setGrid(bool grid) {
    if (_grid != grid) {
        _grid = grid;
        emit gridChanged(grid);
        if (!disableUpdates) {
            // Grid changed, so redraw it on current image
            if (this->xv_format >= 0) makeFullFrame();        
            update();
        }
    }
}

// grid colour
void ffmpegWidget::setGcol(QColor gcol) {
    if (gcol.isValid() && (!_gcol.isValid() || _gcol != gcol)) {
        _gcol = gcol;
        emit gcolChanged(_gcol);
        if (!disableUpdates) {
            // Grid changed, so redraw it on current image
            if (this->xv_format >= 0) makeFullFrame();
            update();
        }
    }
}

// set false colour
void ffmpegWidget::setFcol(int fcol) {
    if (_fcol != fcol) {
        _fcol = fcol;
        emit fcolChanged(_fcol);
        if (!disableUpdates) {
            makeFullFrame();
            update();
        }        
    }
}

// set the URL to connect to
void ffmpegWidget::setUrl(QString url) {
    QString copiedUrl(url);
    if (_url != copiedUrl) {
        _url = copiedUrl;
        emit urlChanged(_url);
        setReset();
    }
}

