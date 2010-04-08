#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QThread>
#include <QMutex>
/* ffmpeg includes */
extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
}

#define NBUFFERS 2

class FFThread : public QThread
{
    Q_OBJECT

public:
    FFThread (const QString &url, QWidget* parent = 0);
    ~FFThread ();
    void run();    

public slots:
    void stopGracefully() { stopping=1; }

signals:
    void updateSignal(QImage *image, bool firstImage);
    
private:
	/* Double buffering */
	unsigned char *frames[NBUFFERS];
	QImage *imgs[NBUFFERS];
    AVFrame             *pictures[NBUFFERS];         	
	int index;
    char *url;
    QByteArray ba;
    int stopping;
    AVFrame             *pFrame;     
};

class VideoItem : public QGraphicsItem
{
public:
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget);
	QImage *image;
    int updateImage(QImage *image);

private:    
	    QMutex *imgmutex; 
};

class VideoWidget : public QGraphicsView
{
	Q_OBJECT
	
	public:
		VideoWidget(const char *url);
		~VideoWidget();
		void wheelEvent(QWheelEvent *event);
		QSize sizeHint () const;

	public slots:
        void zoomIn();
        void zoomOut();
        void reset();        
        void rotateLeft();
        void rotateRight();
        void updateImage(QImage *image, bool firstImage);
	
	signals:
	    void aboutToQuit();
        
	private:
	    FFThread *ff;     
	    void ffQuit(); 	
	    QPixmap *pix; 
	    VideoItem *item;      
	    QColor gcol;
	    QPoint gcentre;
	    int gsize;
	    QGraphicsLineItem *xminor[50];
	    QGraphicsLineItem *yminor[50];	    
	    QGraphicsLineItem *xmajor;
	    QGraphicsLineItem *ymajor;	
    
	    
};



#endif
