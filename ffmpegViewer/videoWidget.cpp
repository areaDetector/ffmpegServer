#include <QWheelEvent>
#include "videoWidget.h"
#include <QGLWidget>
#include <QGLFormat>
#include <QBrush>
#include <QStyleOptionGraphicsItem>

VideoWidget::VideoWidget(const char *url) {

	/* Create a graphics scene */
	QGraphicsScene *scene = new QGraphicsScene(this);

	item = new VideoItem();
	scene->addItem(item);
    this->setScene(scene);
    this->setBackgroundBrush(QBrush(Qt::black));

 	/* Setup lines */
/* 	gcentre = QPoint(300,300);
 	gsize = 40;
 	gcol = QColor(255,255,255,50);
 	
 	int xmax = 300 + 25*gsize;
 	int ymax = 300 + 25*gsize; 	
 	int xmin = 300 - 25*gsize;
 	int ymin = 300 - 25*gsize; 	
 	
 	QPen minorPen(gcol);
 	QPen majorPen(gcol.toRgb());
 	xmajor = new QGraphicsLineItem(xmin, 300, xmax, 300); 	
	xmajor->setPen(majorPen); 	
	scene->addItem(xmajor);
 	ymajor = new QGraphicsLineItem(300, ymin, 300, ymax);
	ymajor->setPen(majorPen); 	
	scene->addItem(ymajor);	
 	
	for (int i=0; i<50; i++) {
		xminor[i] = new QGraphicsLineItem(xmin + i*gsize, ymin, xmin + i*gsize, ymax);
		xminor[i]->setPen(minorPen);
		scene->addItem(xminor[i]);
		yminor[i] = new QGraphicsLineItem(xmin, ymin + i*gsize, xmax, ymin + i*gsize);
		yminor[i]->setPen(minorPen);		
		scene->addItem(yminor[i]);		
	}

    printf("x: %f, y: %f, w: %f, h: %f\n",this->xmajor->boundingRect().x(), this->xmajor->boundingRect().y(), this->xmajor->boundingRect().width(), this->xmajor->boundingRect().height());           */

    /* Setup mouse behaviour */
    this->setDragMode(QGraphicsView::ScrollHandDrag);

    /* OpenGL */
    if (QGLFormat::hasOpenGL()) {
        printf("OpenGL enabled\n");
        this->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
    }

 	/* Launch ffmpeg on the url */
    /* create the ffmpeg thread */
    ff = new FFThread(url, this);
	QObject::connect( ff, SIGNAL(updateSignal(QImage*, bool)),
	                  this, SLOT(updateImage(QImage*, bool)) );
	QObject::connect( this, SIGNAL(aboutToQuit()),
                      ff, SLOT(stopGracefully()) );
    ff->start();
}

QSize VideoWidget::sizeHint () const {
	if (this->item->image) {
		return QSize(this->item->image->width(), this->item->image->height());
	} else {
		return QSize(0,0);
	}	
}

void VideoWidget::updateImage(QImage *image, bool firstImage)
{
    if (this->item->updateImage(image)) {
    	this->scene()->setSceneRect(this->item->boundingRect());
    }
}

void VideoWidget::wheelEvent(QWheelEvent *event)
{
    if (event->delta() > 0) {
    	this->zoomIn();
    } else {
    	this->zoomOut();
    }
}

void VideoWidget::zoomIn() {
	this->scale(1.2, 1.2);
}

void VideoWidget::zoomOut() {
	this->scale(1 / 1.2, 1 / 1.2);
}

void VideoWidget::reset() {
	this->resetTransform();
}

void VideoWidget::rotateLeft() {
	this->rotate(-2);
}

void VideoWidget::rotateRight() {
	this->rotate(2);
}

void VideoWidget::ffQuit() {
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
VideoWidget::~VideoWidget() {
	printf("Destroying ffmpegViewer\n");
    ffQuit();
}

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
	/* Setup double buffering */
	for (index=0; index<NBUFFERS; index++) {
		this->frames[index] = (unsigned char *) malloc(2048*2048*3*sizeof(unsigned char));
		this->imgs[index] = new QImage();
		this->pictures[index] = avcodec_alloc_frame();
	}	
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

}

// destroy widget
FFThread::~FFThread() {
    // free the frames
    av_free(pFrame);
	for (index=0; index<NBUFFERS; index++) {			
		free(this->frames[index]);
	    av_free(this->pictures[index]);   		
	}
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
    PixelFormat         out_fmt, pix_fmt = PIX_FMT_NONE;
    struct SwsContext   *ctx = NULL;
    bool                firstImage = true;

    printf("ffThread started on URL %s\n",url);
//    av_log_set_level(AV_LOG_DEBUG);

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
                this->index = (this->index + 1) % NBUFFERS;
                if((ctx == NULL) || (pix_fmt!=pCodecCtx->pix_fmt) ||
                   (width!=pCodecCtx->width) || (height!=pCodecCtx->height)) {
                    // store pix_fmt, width and height
                    pix_fmt = pCodecCtx -> pix_fmt;
                    width = pCodecCtx -> width;
                    height = pCodecCtx -> height;
                    out_fmt = PIX_FMT_RGB24;
                    // Create a context for software conversion to RGB / GRAY8
                    ctx = sws_getCachedContext(ctx, width, height, pix_fmt,
                                               width, height, out_fmt,
                                               SWS_BICUBIC, NULL, NULL, NULL);
					for (int i=0; i<NBUFFERS; i++) {
	                	// Assign appropriate parts of buffer to planes in pFrameRGB
	    	            avpicture_fill((AVPicture *) this->pictures[i], this->frames[i], out_fmt,
                                pCodecCtx->width, pCodecCtx->height);
                    }
                }

                // Do the software conversion to RGB / GRAY8
                sws_scale(ctx, pFrame->data, pFrame->linesize, 0, height,
                          this->pictures[index]->data, this->pictures[index]->linesize);
                // Parcel it up as a QImage
                imgs[this->index] = new QImage(this->frames[this->index], width, height, QImage::Format_RGB888);
//                printf("Produce: %p %p %d\n", imgs[this->index], this->frames[this->index], this->index);
                // Tell the widget that the picture is ready                
                emit updateSignal(imgs[this->index], firstImage);
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

QRectF VideoItem::boundingRect() const {
    if (this->image) {
	    return QRectF(0, 0, this->image->width(), this->image->height());
	} else {
		return QRectF(0, 0, 0, 0);
	}
}

void VideoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget) {
	if (this->imgmutex==NULL) {
		this->imgmutex=new QMutex();
	}
	this->imgmutex->lock();	            
    if (this->image) {
/*        printf("x: %f, y: %f, w: %f, h: %f\n",this->boundingRect().x(), this->boundingRect().y(), this->boundingRect().width(), this->boundingRect().height());               */
        painter->setClipRect( option->exposedRect );
    	painter->drawImage(0,0,*this->image);
//        printf("Paint: %p\n", this->image);
    }
	this->imgmutex->unlock();		                
}

int VideoItem::updateImage(QImage *image) {
    int updateDims=0;
	if (this->imgmutex==NULL) {
		this->imgmutex=new QMutex();
	}
	this->imgmutex->lock();	    
    if (this->image && (this->image->height() != image->height() || this->image->width() != image->width())) {
        updateDims = 1;
    }
	this->image = image;
	if (updateDims) {
	    this->prepareGeometryChange();
	} else {
    	this->update();	
    }
	this->imgmutex->unlock();	    
    return updateDims;
}

