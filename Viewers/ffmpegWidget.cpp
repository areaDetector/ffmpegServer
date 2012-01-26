#include <QToolTip>
#include <QMutex>
#include "ffmpegWidget.h"
#include <QColorDialog>
#include "colorMaps.h"


#include <assert.h>

/* set this when the ffmpeg lib is initialised */
static int ffinit=0;

/* need this to protect certain ffmpeg functions */
static QMutex *ffmutex;

/* thread that decodes frames from video stream and emits updateSignal when
 * each new frame is available
 */
FFThreadXv::FFThreadXv (const QString &url, unsigned char * destFrame, QWidget* parent)
    : QThread (parent)
{
    /* setup frame */
    // this is the url to read the stream from
    ba = url.toAscii();
    this->url = ba.data();
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
    // Allocate video frame for decoding into
    this->pFrame=avcodec_alloc_frame();
    this->pFrameRGB=avcodec_alloc_frame();  
    this->destFrame = destFrame; 
    this->_fcol = 0;   
}

// destroy widget
FFThreadXv::~FFThreadXv() {
    // free the frames
    av_free(pFrame);
    av_free(pFrameRGB);    
}

// run the FFThreadXv
void FFThreadXv::run()
{
    AVFormatContext     *pFormatCtx;
    int                 videoStream;
    AVCodecContext      *pCodecCtx;
    AVCodec             *pCodec;
    AVPacket            packet;    
    int                 frameFinished, len, width = 0, dstw = 0, height = 0, dsth = 0;
    PixelFormat         pix_fmt = PIX_FMT_NONE;
    struct SwsContext   *ctx = NULL;
    bool                firstImage = true; 

//    printf("ffThread started on URL %s\n",url);
            
    // Open video file
    if(av_open_input_file(&pFormatCtx, url, NULL, 0, NULL)!=0)
        return; // Couldn't open file
        
    // Find the first video stream
    videoStream=-1;
    for(unsigned int i=0; i<pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    }
    if(videoStream==-1)
        return; // Didn't find a video stream
        
    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return; // Codec not found
    }

    // Open codec
    ffmutex->lock();
    if(avcodec_open(pCodecCtx, pCodec)<0) {
        return; // Could not open codec
    }
    ffmutex->unlock();
        
    while ((stopping!=1) && (av_read_frame(pFormatCtx, &packet)>=0)) {        
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream) {
        
            // Decode video frame
            len = avcodec_decode_video2(pCodecCtx, pFrame, 
                                        &frameFinished, &packet);      
            if(frameFinished) {              
                if((ctx == NULL) || (pix_fmt!=pCodecCtx->pix_fmt) || 
                   (width!=pCodecCtx->width) || (height!=pCodecCtx->height))  {    
                    // store pix_fmt, width and height
                    pix_fmt = pCodecCtx -> pix_fmt;
                    width = pCodecCtx -> width;
                    height = pCodecCtx -> height;          
                    // PIX_FMT_YUVJ422P is 0xd
                    printf("width %d height %d pixel format %08x (PIX_FMT_YUVJ420P=%08x)\n", width, height, pix_fmt, PIX_FMT_YUVJ420P);
                    
                    //PixelFormat dest_format = PIX_FMT_YUYV422;
                    PixelFormat dest_format = PIX_FMT_YUVJ420P;
                    
                    // Create a context for software conversion to RGB / GRAY8  
                   	dstw = width - width % 8;
                   	dsth = height - height % 8;
                    ctx = sws_getCachedContext(ctx, width, height, pix_fmt,
                                               width, height, dest_format, 
                                               SWS_BICUBIC, NULL, NULL, NULL);
                    // Assign appropriate parts of buffer to planes in pFrameRGB
                    avpicture_fill((AVPicture *) pFrameRGB, this->destFrame, dest_format,
                                   dstw, dsth);                
                }
                if (this->_fcol) {
                    // Assume we have a YUV input,
                    // Throw away U and V, and use Y to generate RGB using
                    // colourmap
                    const unsigned char * colorMapY, * colorMapU, * colorMapV;
                    switch(this->_fcol) {
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
                    
                    unsigned char *data = (unsigned char *) pFrame->data[0];
                    unsigned char *dest = this->destFrame;
                    for (int h=0; h<dsth; h++) {
                    	unsigned int line_start = pFrame->linesize[0] * h;
                        for (int w=0; w<dstw; w++) {
                        	//this->destFrame[h*dstw + w] = colorMapY[data[line_start + w]];
                        	*dest++ = colorMapY[data[line_start + w]];
                        }                        
                    }

#if 1                    
                    for (int h=0; h<dsth; h+=2) {
                    	unsigned int line_start = pFrame->linesize[0] * h;
                        for (int w=0; w<dstw; w+=2) {
                        	unsigned char y = data[line_start + w];
                        	this->destFrame[dstw*dsth + h*dstw/4 + w/2] = colorMapU[y];
                        	this->destFrame[dstw*dsth + dsth*dstw/4 + h*dstw/4 + w/2] = colorMapV[y];
                        }                        
                    }
#else
                    for (int h=0; h<dsth; h+=2) {
                    	unsigned int line_start = pFrame->linesize[0] * h;
                        for (int w=0; w<dstw; w+=2) {
                        	*dest++ = colorMapU[data[line_start + w]];
                        }                        
                    }
                    for (int h=0; h<dsth; h+=2) {
                    	unsigned int line_start = pFrame->linesize[0] * h;
                        for (int w=0; w<dstw; w+=2) {
                           	*dest++ = colorMapV[data[line_start + w]];
                        }                        
                    }
#endif                    
                } else {
                    // Do the software conversion to RGB / GRAY8
#if 1
                    sws_scale(ctx, pFrame->data, pFrame->linesize, 0, height, 
                              pFrameRGB->data, pFrameRGB->linesize);
#else
                    // real width is pFrame->linesize[0]
                    int ofs = 0;
                    memcpy(destFrame + ofs, pFrame->data[0], VWIDTH * VHEIGHT);
                    ofs += VWIDTH * VHEIGHT;
                    memcpy(destFrame + ofs, pFrame->data[1], VWIDTH / 2 * VHEIGHT / 2);
                    ofs += VWIDTH / 2 * VHEIGHT / 2;
                    memcpy(destFrame + ofs, pFrame->data[2], VWIDTH / 2 * VHEIGHT / 2);
#endif
                    
                }                              
                // Tell the GL widget that the picture is ready
                emit updateSignal(dstw, dsth, firstImage);             
                if (firstImage) firstImage = false;
            } else {
                printf("Frame not finished. Shouldn't see this...\n");
            }
        } else {
            printf("Non video packet. Shouldn't see this...\n");
        }
        // Free the packet
        av_free_packet(&packet);           
    }
    // tidy up    
    ffmutex->lock();    
    
    avcodec_close(pCodecCtx);

    av_close_input_file(pFormatCtx);    
//    av_free(pCodecCtx);
    
//    av_free(pFormatCtx);
    pCodecCtx = NULL;

    pFormatCtx = NULL;
    
    ffmutex->unlock();        
    
}

ffmpegWidget::ffmpegWidget (const QString &url, QWidget* parent)
    : QWidget (parent), bmp(NULL)
{
    init();
    setUrl(url);
    ffInit();
}

ffmpegWidget::ffmpegWidget (QWidget* parent)
    : QWidget (parent), bmp(NULL)
{
    init();
}

void ffmpegWidget::init() {
    /* setup some defaults, we'll overwrite them with sensible numbers later */
    _x = 0;
    _y = 0; 
    _w = 0;
    _h = 0;
    _maxX = 0;
    _maxY = 0;     
    _zoom = 0;
    _gx = 100;
    _gy = 100;
    _maxGx = 0;
    _maxGy = 0;
    _fps = 0.0;
    _imw = 0;
    _imh = 0;      
    _grid = false;
    _url = QString("");  
    sf = 1.0;
    ff = NULL;
    lastFrameTime = NULL;
    tex = 0;
    this->destFrame = (unsigned char *) calloc(MAXWIDTH*MAXHEIGHT*3, sizeof(unsigned char));   
    xvsetup();
    setGcol(255, 255, 255);
    tickindex = 0;
    ticksum = 0;
    for (int i=0; i<MAXTICKS; i++) {
    	ticklist[i] = 0;
    }
    this->timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(calcFps()));
    this->timer->start(100);
}    

void ffmpegWidget::xvsetup()
{
    XvAdaptorInfo * ainfo;
    qX11Info = x11Info();
    dpy = qX11Info.display();
    unsigned j, adaptors;
    assert(XvQueryExtension(dpy, &j, &j, &j, &j, &j) == Success);
    assert(XvQueryAdaptors(dpy, QX11Info::appRootWindow(), 
                           &adaptors, &ainfo) == Success);	                           
    xv_port = ainfo[0].base_id;
    for(int p = 0; p < (int)ainfo[0].num_ports; p++)
    {
        if(XvGrabPort(dpy, xv_port, CurrentTime) == Success)
        {
            printf("got port %d\n", p);
            break;
        }
        xv_port++;
    }
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_OpaquePaintEvent);
    w = winId();
    gc = XCreateGC(dpy, w, 0, 0);    
    int num_formats = 0;
    XvImageFormatValues * vals = XvListImageFormats(dpy, xv_port, &num_formats);
    for (int i=0; i<num_formats; i++) {
    	printf("Found format %s\n", vals->guid);
    	vals++;
    }
}


void ffmpegWidget::updateImage(int imw, int imh, bool firstImage) 
{
    // time since last frame    
    if (this->lastFrameTime == NULL) {
    	this->lastFrameTime = new QTime();
	    this->lastFrameTime->start();    	    	
    } else {
    	int elapsed = this->lastFrameTime->elapsed();    	
	    this->lastFrameTime->start();    	
	    //if (elapsed < 20) return;
    	ticksum-=ticklist[tickindex];  /* subtract value falling off */
	    ticksum+=elapsed;              /* add new value */
	    ticklist[tickindex]=elapsed;   /* save new value so it can be subtracted later */
	    if(++tickindex==MAXTICKS)      /* inc buffer index */
    	    tickindex=0;
    	this->_fps = 1000.0  * MAXTICKS / ticksum;
    	emit fpsChanged(QString("%1").arg(this->_fps, 0, 'f', 1));
    }

    // if this is the first image, then make sure we zoom onto it
    if (firstImage || _imw != imw || _imh != imh) {
        _imw = imw;
        _imh = imh;
	    if (_imw <= 0 || _imh <= 0) {
	    	return;
	    }
	    /* Zoom so it fills the viewport */        
	    disableUpdates = true;            
	    setX(0);
    	setY(0);
	    setZoom(0);    
		/* Create an XImage to wrap the data */        
    	enum { SDL_YV12_OVERLAY = 0x32315659 };
	    enum { SDL_I420_OVERLAY = 0x30323449 };
	    enum { SDL_YUY2_OVERLAY = 0x32595559 };     
	    if (bmp) XFree(bmp);       
        bmp = XvCreateImage(dpy, xv_port, SDL_I420_OVERLAY, 0, _imw, _imh);        
	    assert(bmp);
    	bmp->data = (char *)destFrame;
    	/* Update the maximum gridx and gridy values */
        _maxGx = _imw - 1;
        emit maxGxChanged(_maxGx);
        _maxGy = _imh - 1;
        emit maxGyChanged(_maxGy);   
        /* Allow updates again */
	    disableUpdates = false;                          
	    updateScalefactor();
    }
    update();

}

void ffmpegWidget::updateScalefactor() {
	/* Width or height of window has changed, so update scale */
	_w = width();
	_h = height();    	
    /* Work out which is the minimum ratio to scale by */
    double wratio = _w / (double) _imw;
    double hratio = _h / (double) _imh;
    sf = pow(10, (double) (_zoom)/ 20) * qMin(wratio, hratio);     
    /* Now work out how much of the image we need to scale */
    xvwidth = qMin((int) (_imw*sf + 0.5), _w);    
    xvheight = qMin((int) (_imh*sf + 0.5), _h);
    srcw = qMin((int) (xvwidth / sf + 0.5), _imw);
    srch = qMin((int) (xvheight / sf + 0.5), _imh);   
    emit displayedWChanged(QString("%1").arg(srcw));
    emit displayedHChanged(QString("%1").arg(srch));    
    /* Now work out max x and y */;    
    int maxX = qMax(_imw - srcw, 0);
    int maxY = qMax(_imh - srch, 0);
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
    /* Clear area not filled by image */
    if (xvwidth < _w) XClearArea(dpy, w, xvwidth, 0, _w-xvwidth, _h, 0);
    if (xvheight < _h) XClearArea(dpy, w, 0, xvheight, _w, _h-xvheight, 0);    
}       
 
void ffmpegWidget::paintEvent(QPaintEvent *) {
    if (_imw <= 0 || _imh <= 0) {
    	return;
    }      
    if (_w != width() || _h != height()) {
    	updateScalefactor();
    }   
    /* Draw the image */
    XvPutImage(dpy, xv_port, w, gc, bmp, _x, _y, srcw, srch, 0, 0, xvwidth, xvheight);
    /* Draw the grid */
    if (_grid) {
    	int xvgx = (int) ((_gx-_x)*sf);
    	int xvgy = (int) ((_gy-_y)*sf);    	
    	XSetForeground(dpy, gc, _gcol.pixel);
    	if (xvgx >= 0) XDrawLine(dpy, w, gc, xvgx, 0, xvgx, xvheight);
    	if (xvgy >= 0) XDrawLine(dpy, w, gc, 0, xvgy, xvwidth, xvgy);    	
    }
}

void ffmpegWidget::calcFps() {
	if (this->lastFrameTime != NULL) {
		if (this->lastFrameTime->elapsed() > 1500.0 / this->_fps) {
	    	emit fpsChanged(QString("0.0"));
	    }
	}
}

// Initialise the ffthread
void ffmpegWidget::ffInit() { 
    // must have a url
    int fcol = 0;
    if (_url=="") return;
    if ((ff!=NULL)&&(ff->isRunning())) {
    	fcol = ff->fcol();
    	ffQuit();
    }

    // first make sure we don't update anything too quickly
    disableUpdates = true;    
    
    /* create the ffmpeg thread */
    ff = new FFThreadXv(_url, this->destFrame, this);    
    QObject::connect( ff, SIGNAL(updateSignal(int, int, bool)), 
                      this, SLOT(updateImage(int, int, bool)) );
    QObject::connect( this, SIGNAL(aboutToQuit()),
                      ff, SLOT(stopGracefully()) );                       
                      
    // allow GL updates, and start the image thread
    disableUpdates = false;                              
    ff->start(); 
    ff->setFcol(fcol);
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

// destroy widget
ffmpegWidget::~ffmpegWidget() {
    ffQuit();
    free(destFrame);            
}

void ffmpegWidget::setReset() {
	ffInit();
}

// update grid centre
void ffmpegWidget::mouseDoubleClickEvent (QMouseEvent* event) {   
    if (event->button() == Qt::LeftButton) {
        disableUpdates = true;     
        setGx((int) (_x + event->x()/sf + 0.5));
        setGy((int) (_y + event->y()/sf + 0.5));
        disableUpdates = false;            
        event->accept();
        update();
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
     } else if (event->button() == Qt::RightButton) {
         setGrid(!_grid);   
         event->accept();
     } else if (event->button() == Qt::MidButton) {
         QToolTip::showText(event->globalPos(), _url, this);
     }
}

// drag the screen round so the pixel "grabbed" stays under the cursor
void ffmpegWidget::mouseMoveEvent (QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        // disable automatic updates
        disableUpdates = true;
        setX(oldx + (int)((clickx - event->x())/sf));
        setY(oldy + (int)((clicky - event->y())/sf));  
        disableUpdates = false;            
        update();
        event->accept();
    }
}

// zoom in and out on the cursor
void ffmpegWidget::wheelEvent(QWheelEvent * event) {
    // disable automatic updates
    disableUpdates = true;
    // this is the fraction of screen dims that mouse is across
    double fx = event->x()/(double)xvwidth;
    double fy = event->y()/(double)xvheight;
    int oldsrcw = srcw;
    int oldsrch = srch;
    if (event->delta() > 0) {
        this->setZoom(this->zoom() + 1);
    } else {
        this->setZoom(this->zoom() - 1);
    }
    // now work out where it is when zoom has changed
    setX(_x + (int) (0.5 + fx * (oldsrcw - srcw)));
    setY(_y + (int) (0.5 + fy * (oldsrch - srch)));    
    disableUpdates = false;                   
    update();
    event->accept();
} 


// x offset of current frame from origin of full size image
void ffmpegWidget::setX(int x) { 
    x = x < 0 ? 0 : (x > _maxX) ? _maxX : x;
    if (_x != x) { 
        _x = x;
        emit xChanged(x);
        if (!disableUpdates) {        
            update();
        }
    }
}

// y offset of current frame from origin of full size image
void ffmpegWidget::setY(int y) { 
    y = y < 0 ? 0 : (y > _maxY) ? _maxY : y;   
    if (_y != y) { 
        _y = y;
        emit yChanged(y);
        if (!disableUpdates) {     
            update();
        }
    }
}

// zoom frame
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

// x offset of grid centre from origin of full size image
void ffmpegWidget::setGx(int gx) {
    gx = (gx < 1) ? 1 : (gx > _imw - 1 && _imw > 0) ? _imw - 1 : gx;
    if (_gx != gx) { 
        _gx = gx; 
        emit gxChanged(gx);
        if (!disableUpdates) {
            update();
        }
    }
}

// y offset of grid centre from origin of full size image
void ffmpegWidget::setGy(int gy) {
    gy = (gy < 1) ? 1 : (gy > _imh - 1 && _imh > 0) ? _imh - 1 : gy;
    if (_gy != gy) { 
        _gy = gy; 
        emit gyChanged(gy);
        if (!disableUpdates) {
            update();
        }
    }
}

// set the grid colour, RGB will all be set to this, 0 <= gcol < 256
void ffmpegWidget::setGcol() {
    QColor color = QColorDialog::getColor(QColor(_r, _g, _b), this, QString("Choose grid Colour"));
    if (color.isValid()) {
	    setGcol(color.red(), color.green(), color.blue());
	}
}

// set the grid colour, RGB will all be set to this, 0 <= gcol < 256
void ffmpegWidget::setGcol(int r, int g, int b) {
    if (_r != r || _g != g || _b != b) {
    	_r = r;
    	_g = g;
    	_b = b;
        emit gcolChanged(r, g, b);
        Colormap colormap = DefaultColormap(dpy, 0);
        char colString[255];
        snprintf(colString, 255, "#%x%x%x",r,g,b);
        printf("%s\n", colString);
        XParseColor(dpy, colormap, colString, &_gcol);
        XAllocColor(dpy, colormap, &_gcol);
        if (!disableUpdates) {        
            update();
        }
    }
}

// turn the grid on or off
void ffmpegWidget::setGrid(bool grid) { 
    if (_grid != grid) { 
        _grid = grid;
        emit gridChanged(grid);
        if (!disableUpdates) {
            update();
        }
    }
}

// set the URL to connect to
void ffmpegWidget::setUrl(const QString &url) { 
    QString copiedUrl(url);
    if (_url != copiedUrl) { 
        _url = copiedUrl;
        emit urlChanged(_url);     
//        ffInit(); 
    }
}    

