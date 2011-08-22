#include <QToolTip>
#include <QMutex>
#include "ffmpegViewer.h"
#include <QColorDialog>
#include "colorMaps.h"

/* set this when the ffmpeg lib is initialised */
static int ffinit=0;

/* need this to protect certain ffmpeg functions */
static QMutex *ffmutex;

/* thread that decodes frames from video stream and emits updateSignal when
 * each new frame is available
 */
FFThread::FFThread (const QString &url, unsigned char * destFrame, QWidget* parent)
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
FFThread::~FFThread() {
    // free the frames
    av_free(pFrame);
    av_free(pFrameRGB);    
}

// run the FFThread
void FFThread::run()
{
    AVFormatContext     *pFormatCtx;
    int                 videoStream;
    AVCodecContext      *pCodecCtx;
    AVCodec             *pCodec;
    AVPacket            packet;    
    int                 frameFinished, len, width = 0, height = 0;
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
    av_dump_format(pFormatCtx, 0, url, 0);
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
                    // Create a context for software conversion to RGB / GRAY8                
                    ctx = sws_getCachedContext(ctx, width, height, pix_fmt,
                                               width, height, PIX_FMT_RGB24, 
                                               SWS_BICUBIC, NULL, NULL, NULL);
                    // Assign appropriate parts of buffer to planes in pFrameRGB
                    avpicture_fill((AVPicture *) pFrameRGB, this->destFrame, PIX_FMT_RGB24,
                                   width, height);                
                }
                if (this->_fcol) {
                    // Assume we have a YUV input,
                    // Throw away U and V, and use Y to generate RGB using
                    // colourmap
                    const unsigned char * colorMap;
                    switch(this->_fcol) {
                        case 1:
                            colorMap = RainbowColorRGB;
                            break;
                        case 2:
                            colorMap = IronColorRGB;
                            break;
                        default:
                            colorMap = RainbowColorRGB;
                            break;
                    }                            
                    for (int h=0; h<height; h++) {
                        for (int w=0; w<width; w++) {
                            memcpy(this->destFrame + 3*(h*width + w), colorMap + 3 * *(((unsigned char *)pFrame->data[0])+(pFrame->linesize[0]*h) + w), 3);
                        }
                    }
                } else {
                    // Do the software conversion to RGB / GRAY8
                    sws_scale(ctx, pFrame->data, pFrame->linesize, 0, height, 
                              pFrameRGB->data, pFrameRGB->linesize);
                }                              
                // Tell the GL widget that the picture is ready
                emit updateSignal(width, height, firstImage);             
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

ffmpegViewer::ffmpegViewer (const QString &url, QWidget* parent)
    : QGLWidget (parent)
{
    init();
    setUrl(url);
    ffInit();
}

ffmpegViewer::ffmpegViewer (QWidget* parent)
    : QGLWidget (parent)
{
    init();
}

void ffmpegViewer::init() {
    /* setup some defaults, we'll overwrite them with sensible numbers later */
    _x = 0;
    _y = 0; 
    _maxX = 0;
    _maxY = 0;     
    _zoom = 0;
    _gx = 100;
    _gy = 100;
    _maxGx = 0;
    _maxGy = 0;
    _gs = 20;    
    _fps = 0.0;
    _gcol = Qt::white;   
    _gcol.setAlpha(50); 
    _w = 0;
    _h = 0;         
    _imw = 0;
    _imh = 0;      
    _grid = false;
    _url = QString("");  
    ff = NULL;
    lastFrameTime = NULL;
    tex = 0;
    this->destFrame = (unsigned char *) calloc(MAXWIDTH*MAXHEIGHT*3, sizeof(unsigned char));   
    this->timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(calcFps()));
    this->timer->start(100);
}    

void ffmpegViewer::calcFps() {
	if (this->lastFrameTime != NULL) {
		if (this->lastFrameTime->elapsed() > 1500.0 / this->_fps) {
	    	emit fpsChanged(QString("0.0"));
	    }
	}
}

// Initialise the ffthread
void ffmpegViewer::ffInit() { 
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
    ff = new FFThread(_url, this->destFrame, this);    
    QObject::connect( ff, SIGNAL(updateSignal(int, int, bool)), 
                      this, SLOT(updateImage(int, int, bool)) );
    QObject::connect( this, SIGNAL(aboutToQuit()),
                      ff, SLOT(stopGracefully()) );                       
                      
    // allow GL updates, and start the image thread
    disableUpdates = false;                              
    ff->start(); 
    ff->setFcol(fcol);
}

void ffmpegViewer::ffQuit() {
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
ffmpegViewer::~ffmpegViewer() {
    ffQuit();
    free(destFrame);            
}


// x offset of current frame from origin of full size image
void ffmpegViewer::setX(int x) { 
    x = (x < 0 || _imw < _w) ? 0 : (x > _imw - _w) ? _imw - _w : x;
    if (_x != x) { 
        _x = x;
        emit xChanged(x);
        if (!disableUpdates) {        
            updateViewport();    
            updateGL();
        }
    }
}

// y offset of current frame from origin of full size image
void ffmpegViewer::setY(int y) { 
    y = (y < 0|| _imh < _h) ? 0 : (y > _imh - _h) ? _imh - _h : y;   
    if (_y != y) { 
        _y = y;
        emit yChanged(y);
        if (!disableUpdates) {     
            updateViewport();    
            updateGL();
        }
    }
}

// zoom frame
void ffmpegViewer::setZoom(int zoom) { 
    zoom = (zoom < -30) ? -30 : (zoom > 30) ? 30 : zoom;   
    if (_zoom != zoom) { 
        _zoom = zoom;
        emit zoomChanged(zoom);
        if (!disableUpdates) {   
            initializeGL();          
            updateViewport();    
            updateGL();
        }
    }
}

// x offset of grid centre from origin of full size image
void ffmpegViewer::setGx(int gx) {
    gx = (gx < 1) ? 1 : (gx > _imw - 1 && _imw > 0) ? _imw - 1 : gx;
    if (_gx != gx) { 
        _gx = gx; 
        emit gxChanged(gx);
        if (!disableUpdates) {
            updateGL();
        }
    }
}

// y offset of grid centre from origin of full size image
void ffmpegViewer::setGy(int gy) {
    gy = (gy < 1) ? 1 : (gy > _imh - 1 && _imh > 0) ? _imh - 1 : gy;
    if (_gy != gy) { 
        _gy = gy; 
        emit gyChanged(gy);
        if (!disableUpdates) {
            updateGL();
        }
    }
}

// pixel width of gridlines from grid centre outwards
void ffmpegViewer::setGs(int gs) {
    gs = (gs < 2) ? 2 : (gs > _imw && _imw > 0) ? _imw : gs;
    if (_gs!=gs) { 
        _gs = gs;        
        emit gsChanged(gs); 
        if (!disableUpdates) {        
            updateGL();
        }
    }
}            

// set the grid colour, RGB will all be set to this, 0 <= gcol < 256
void ffmpegViewer::setGcol() {
    QColor color = QColorDialog::getColor(_gcol, this, QString("Choose grid Colour"), QColorDialog::ShowAlphaChannel);
    if (color.isValid() and _gcol != color) {
        _gcol = color;
//        emit gcolChanged(gcol);
        if (!disableUpdates) {        
            updateGL();
        }
    }
}

// turn the grid on or off
void ffmpegViewer::setGrid(bool grid) { 
    if (_grid != grid) { 
        _grid = grid;
        emit gridChanged(grid);
        if (!disableUpdates) {
            updateGL();
        }
    }
}

// set the URL to connect to
void ffmpegViewer::setUrl(const QString &url) { 
    QString copiedUrl(url);
    if (_url != copiedUrl) { 
        _url = copiedUrl;
        emit urlChanged(_url);     
//        ffInit(); 
    }
}

void ffmpegViewer::initializeGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth (1.0f);
    glClear (GL_COLOR_BUFFER_BIT);
    // enable blending
    glEnable (GL_BLEND); 
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (tex) {
        glDeleteTextures(1, &tex);        
    }
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    // select replace to just draw the texture
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // enable texture smoothing
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _imw, _imh, 0, GL_RGB, 
                 GL_UNSIGNED_BYTE, destFrame);  
    // compile single textured quad
    qlist = glGenLists(1);
    glNewList(qlist, GL_COMPILE);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0);    glVertex2f(0.0, 1.0);
    glTexCoord2f(1.0, 0.0);    glVertex2f(1.0, 1.0);
    glTexCoord2f(1.0, 1.0);    glVertex2f(1.0, 0.0);
    glTexCoord2f(0.0, 1.0);    glVertex2f(0.0, 0.0);
    glEnd();
    glEndList();    
}

void ffmpegViewer::resizeGL (int width, int height) {
    // just set the current viewport to the size of the widget
    glViewport(0, 0, width, height);
    updateViewport();            
}

void ffmpegViewer::setReset() {
	ffInit();
}

void ffmpegViewer::zoomOntoImage() {
    // zoom onto the image
    makeCurrent();
    disableUpdates = true;
    setX(0);
    setY(0);
    setZoom(-30);    
    disableUpdates = false;     
    initializeGL();
    updateViewport();
    updateGL();
}

void ffmpegViewer::updateImage(int imw, int imh, bool firstImage) 
{
    // make sure we update the current OpenGL window
    makeCurrent();
    // draw the subimage onto the texture        
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imw, imh, GL_RGB, GL_UNSIGNED_BYTE, destFrame);     
    
    // time since last frame    
    if (this->lastFrameTime == NULL) {
    	this->lastFrameTime = new QTime();
    } else {
    	this->_fps = (1000.0 / this->lastFrameTime->elapsed()) * 0.5 + this->_fps * 0.5;
    	emit fpsChanged(QString("%1").arg(this->_fps));
    }
    this->lastFrameTime->start();

    // if this is the first image, then make sure we zoom onto it
    if (firstImage || _imw != imw || _imh != imh) {
        _imw = imw;
        _imh = imh;        
        zoomOntoImage();
        _maxGx = _imw - 1;
        emit maxGxChanged(_maxGx);
        _maxGy = _imh - 1;
        emit maxGyChanged(_maxGy);                
    } else {        
        updateGL();
    }
}

void ffmpegViewer::paintGL() {

    // draw quad with image texture
    glEnable(GL_LIGHTING);    
    glEnable(GL_TEXTURE_2D);    
    glCallList(qlist);    
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    /* Render grid overlay */    
    if (_grid) {
        float i;    
        float xcentref = (float) _gx / (float) _imw;
        float ycentref = (float) _gy / (float) _imh;
        // express this as a fraction of maxW and maxH
        float gspacex = (float) _gs / (float) _imw;
        float gspacey = (float) _gs / (float) _imh;               
        glBegin(GL_LINES);  
        // first draw the major axes solid
        glColor4f(_gcol.redF(), _gcol.greenF(), _gcol.blueF(), 1.0);
        // horizontal
        glVertex2f(0.0, 1.0 - ycentref);
        glVertex2f(1.0, 1.0 - ycentref);
        // vertical
        glVertex2f(xcentref, 0.0);
        glVertex2f(xcentref, 1.0);
        // now do the minor axes semi-transparent
        glColor4f(_gcol.redF(), _gcol.greenF(), _gcol.blueF(), _gcol.alphaF());
        // horizontal below the major axis
        for (i = 1.0 - ycentref - gspacey; i > 0.0; i -= gspacey) {
            glVertex2f(0.0, i);
            glVertex2f(1.0, i);
        }
        // horizontal above the major axis        
        for (i = 1.0 - ycentref + gspacey; i < 1.0; i += gspacey) {
            glVertex2f(0.0, i);
            glVertex2f(1.0, i);
        }
        // vertical to the left of the major axis
        for (i = xcentref - gspacex; i > 0.0; i -= gspacex) {
            glVertex2f(i, 0.0);
            glVertex2f(i, 1.0);
        }
        // vertical to the right of the major axis        
        for (i = xcentref + gspacex; i < 1.0; i += gspacex) {
            glVertex2f(i, 0.0);
            glVertex2f(i, 1.0);
        }
        glEnd();
    }
    glFlush ();    
}

// update grid centre
void ffmpegViewer::mouseDoubleClickEvent (QMouseEvent* event) {   
    if (event->button() == Qt::LeftButton) {
        disableUpdates = true;     
        setGx((int) (_x + _w*((float) event->x())/width()));
        setGy((int) (_y + _h*((float) event->y())/height()));
        disableUpdates = false;            
        event->accept();
        updateGL();
    }
}

// store the co-ordinates of the original mouse click and the old x and y
void ffmpegViewer::mousePressEvent (QMouseEvent* event) {   
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
void ffmpegViewer::mouseMoveEvent (QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        // disable automatic updates
        disableUpdates = true;
        setX(oldx + (int)((float)(clickx - event->x())/width() * _w));
        setY(oldy + (int)((float)(clicky - event->y())/height() * _h));  
        disableUpdates = false;            
        updateViewport();    
        updateGL();
        event->accept();
    }
}

// zoom in and out on the cursor
void ffmpegViewer::wheelEvent(QWheelEvent * event) {
    int oldw = _w, oldh = _h;
    // disable automatic updates
    disableUpdates = true;   
    if (event->delta() > 0) {
        this->setZoom(this->zoom() + 1);
    } else {
        this->setZoom(this->zoom() - 1);
    }
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth (1.0f);
    glClear (GL_COLOR_BUFFER_BIT);
    initializeGL();
    updateViewport();    
    setX(_x + (int) (0.5 + ((float) (oldw - _w)*event->x()) / width()));
    setY(_y + (int) (0.5 + ((float) (oldh - _h)*event->y()) / height())); 
    updateViewport();    
    disableUpdates = false;                   
    updateGL();
    event->accept();
}     

// update the viewport with new w, h, x, y
void ffmpegViewer::updateViewport() {  
//    initializeGL();
//    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   
	int newzoom = _zoom + 1;
	double newsf = pow(10, (double) (newzoom)/ 20); 		
    if (_imw > 0 && _imh > 0) {
    	// work out new scale factor
    	while (_imw * newsf < width() && _imh * newsf < height()) {
    		newzoom += 1;
	    	newsf = pow(10, (double) (newzoom)/ 20);     		
    	}
    }
    if (newzoom - 1 > _zoom) {
    	_zoom = newzoom - 1;
        emit zoomChanged(_zoom);                	
    }    
    double sf = pow(10, (double) _zoom / 20);    
    _w = (int) (width() / sf);
    _h = (int) (height() / sf);    
    int maxX = qMax(_imw - _w, 0);
    int maxY = qMax(_imh - _h, 0);
    if (_maxX != maxX) {
        _maxX = maxX;
        emit maxXChanged(_maxX);
    }
    if (_maxY != maxY) {
        _maxY = maxY;
        emit maxYChanged(_maxY);
    }    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();        
    gluOrtho2D( (float) _x / (float) _imw, 
                ((float) _x + (float) _w) / (float) _imw, 
                1.0 - ((float) _y + (float) _h) / (float) _imh, 
                1.0 - (float) _y / (float) _imh);        
}
