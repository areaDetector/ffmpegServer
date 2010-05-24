/*#include <stdio.h>
#include <math.h>
#include <stdlib.h>*/
#include <QToolTip>
#include <QMutex>
#include "ffmpegViewer.h"
#include <QColorDialog>

/* set this when the ffmpeg lib is initialised */
static int ffinit=0;

/* need this to protect certain ffmpeg functions */
static QMutex *ffmutex;

/* thread that decodes frames from video stream and emits updateSignal when
 * each new frame is available
 */
FFThread::FFThread (const QString &url, QWidget* parent)
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
	this->destFrame = (unsigned char *) malloc(MAXWIDTH*MAXHEIGHT*3*sizeof(unsigned char));    
}

// destroy widget
FFThread::~FFThread() {
    // free the frames
    free(destFrame);        
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

    printf("ffThread started on URL %s\n",url);
            
    // Open video file
    if(av_open_input_file(&pFormatCtx, url, NULL, 0, NULL)!=0)
        return; // Couldn't open file
        
    // Find the first video stream
    videoStream=-1;
    for(unsigned int i=0; i<pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
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
                    // Create a context for software conversion to RGB / GRAY8                
                    ctx = sws_getCachedContext(ctx, width, height, pix_fmt,
                                               width, height, PIX_FMT_RGB24, 
                                               SWS_BICUBIC, NULL, NULL, NULL);
                    // Assign appropriate parts of buffer to planes in pFrameRGB
                    avpicture_fill((AVPicture *) pFrameRGB, this->destFrame, PIX_FMT_RGB24,
                                   width, height);                
                }
                // Do the software conversion to RGB / GRAY8
                sws_scale(ctx, pFrame->data, pFrame->linesize, 0, height, 
                          pFrameRGB->data, pFrameRGB->linesize);
                // Tell the GL widget that the picture is ready
                emit updateSignal(width, height, firstImage, (void*) this->pFrameRGB->data[0]);             
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
    printf("ffThread finished\n");
    
}

ffmpegViewer::ffmpegViewer (const QString &url, QWidget* parent)
	: QGLWidget (parent)
{
	printf("Creating ffmpegViewer with url\n");
    init();
    setUrl(url);
    ffInit();
}

ffmpegViewer::ffmpegViewer (QWidget* parent)
	: QGLWidget (parent)
{
	printf("Creating ffmpegViewer\n");
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
    _gs = 20;    
    _gcol = Qt::black;   
    _gcol.setAlpha(50); 
    _w = 0;
    _h = 0;         
    _imw = 0;
    _imh = 0;      
    _grid = false;
    _url = QString("");  
    ff = NULL;
}    


// Initialise the ffthread
void ffmpegViewer::ffInit() { 
    // must have a url
    if (_url=="") return;
    if ((ff!=NULL)&&(ff->isRunning())) ffQuit();

    printf("ffInit\n");

    // first make sure we don't update anything too quickly
    disableUpdates = true;    
    
    /* create the ffmpeg thread */
    ff = new FFThread(_url, this);    
	QObject::connect( ff, SIGNAL(updateSignal(int, int, bool, void*)), 
	                  this, SLOT(updateImage(int, int, bool, void*)) );
	QObject::connect( this, SIGNAL(aboutToQuit()),
                      ff, SLOT(stopGracefully()) ); 	                  
                      
    // allow GL updates, and start the image thread
    disableUpdates = false;                              
    ff->start(); 
}

void ffmpegViewer::ffQuit() {
	printf("ffQuit\n");
    // Tell the ff thread to stop
    if (ff==NULL) return;
    emit aboutToQuit();
    if (!ff->wait(500)) {
        printf("Kill unresponsive ffThread\n");
        // thread won't stop, kill it
        ff->terminate();
        ff->wait();   
    }    
    delete ff;
    ff = NULL;
}    

// destroy widget
ffmpegViewer::~ffmpegViewer() {
	printf("Destroying ffmpegViewer\n");
    ffQuit();
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
    zoom = (zoom < -100) ? -100 : (zoom > 100) ? 100 : zoom;   
    if (_zoom != zoom) { 
        _zoom = zoom;
        emit zoomChanged(zoom);
        if (!disableUpdates) {     
            updateViewport();    
            updateGL();
        }
    }
}

// x offset of grid centre from origin of full size image
void ffmpegViewer::setGx(int gx) {
    gx = (gx < 0) ? 0 : (gx > _imw) ? _imw : gx;
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
    gy = (gy < 0) ? 0 : (gy > _imh) ? _imh : gy;
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
    gs = (gs < 2) ? 2 : (gs > _imw) ? _imw : gs;
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
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClearDepth (1.0f);
//    glClear(); 
	// enable blending
    glEnable (GL_BLEND); 
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		
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
    // zoom onto the image
    makeCurrent();
    _x = 0;
    _y = 0;
    _zoom = 0;
    initializeGL();
    updateViewport();
    updateGL();

}

void ffmpegViewer::updateImage(int imw, int imh, bool firstImage, void *data) 
{
    unsigned char *destFrame = (unsigned char*) data;
    // make sure we update the current OpenGL window
    makeCurrent();
    _imw = imw;
    _imh = imh;
    // draw the subimage onto the texture        
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imw, imh, GL_RGB, GL_UNSIGNED_BYTE, destFrame);          

    // if this is the first image, then make sure we zoom onto it
    if (firstImage) {
        setReset();
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
    updateViewport();	
    setX(_x + (int) (((float) (oldw - _w)*event->x()) / width()));
    setY(_y + (int) (((float) (oldh - _h)*event->y()) / height())); 
    disableUpdates = false;                   
    updateGL();
    event->accept();
}     

// update the viewport with new w, h, x, y
void ffmpegViewer::updateViewport() {  
//    initializeGL();
//    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);    
    double sf = pow(2, (double) _zoom / 10);
    _w = (int) (width() / sf);
    _h = (int) (height() / sf);    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();        
    gluOrtho2D( (float) _x / (float) _imw, 
                ((float) _x + (float) _w) / (float) _imw, 
                1.0 - ((float) _y + (float) _h) / (float) _imh, 
                1.0 - (float) _y / (float) _imh);        
}

       
