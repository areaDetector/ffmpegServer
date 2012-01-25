#ifndef FFMPEGWIDGET_H
#define FFMPEGWIDGET_H

#include <QThread>
#include <QWheelEvent>
#include <QtDesigner/QDesignerExportWidget>
#include <QSpinBox>
#include <QTime>
#include <QTimer>
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>

/* ffmpeg includes */
extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
}

#define MAXWIDTH 4000
#define MAXHEIGHT 3000

class FFThreadXv : public QThread
{
    Q_OBJECT

public:
    FFThreadXv (const QString &url, unsigned char* destFrame, QWidget* parent = 0);
    ~FFThreadXv ();
    void run();    
    int fcol() { return _fcol; }

public slots:
    void stopGracefully() { stopping=1; }
    void setFcol(int x) { _fcol = x; }
    
signals:
    void updateSignal(int imw, int imh, bool firstImage);
    
private:
    char *url;
    QByteArray ba;
    int stopping, maxW, maxH;
    AVFrame             *pFrame;     
    AVFrame             *pFrameRGB;         
    uint8_t             *buffer;    
    unsigned char* destFrame;
    int _fcol;
};

class QDESIGNER_WIDGET_EXPORT ffmpegWidget : public QWidget
{
	Q_OBJECT
    Q_PROPERTY( int x READ x WRITE setX)
    Q_PROPERTY( int y READ y WRITE setY)
    Q_PROPERTY( int zoom READ zoom WRITE setZoom)    
    Q_PROPERTY( int gx READ gx WRITE setGx)
    Q_PROPERTY( int gy READ gy WRITE setGy)
    Q_PROPERTY( int gs READ gs WRITE setGs)
    Q_PROPERTY( bool grid READ grid WRITE setGrid)    
    Q_PROPERTY( QString url READ url WRITE setUrl)  
          

public:
	ffmpegWidget (QWidget* parent = 0);
	ffmpegWidget (const QString &url, QWidget* parent = 0);
	~ffmpegWidget ();

    int x() const           { return _x; }
    int y() const           { return _y; }
    int maxX() const        { return _maxX; }
    int maxY() const        { return _maxY; }            
    int zoom() const        { return _zoom; }    
    int gx() const          { return _gx; }
    int gy() const          { return _gy; }
    int maxGx() const       { return _maxGx; }
    int maxGy() const       { return _maxGy; }                
    int gs() const          { return _gs; }
    bool grid() const       { return _grid; }
    QString url() const     { return _url; }
    
signals:
    void xChanged(int x);
    void yChanged(int y);
    void maxXChanged(int maxX);
    void maxYChanged(int maxY);            
    void zoomChanged(int zoom);    
    void gxChanged(int gx);
    void gyChanged(int gy);
    void maxGxChanged(int maxGx);
    void maxGyChanged(int maxGy);                
    void gsChanged(int gs);    
    void gcolChanged(int r, int g, int b);
    void gridChanged(bool grid);
    void urlChanged(const QString &);
    void fpsChanged(const QString &);
    void aboutToQuit();

public slots:
    void setX(int x);
    void setY(int y);
    void setZoom(int zoom);    
    void setGx(int gx);
    void setGy(int gy);
    void setGs(int gs);
    void setGcol();
    void setGcol(int r, int g, int b);    
    void setGrid(bool grid);    
    void setUrl(const QString &url);
    void setReset();    
    void ffInit();
	void updateImage (int imw, int imh, bool firstImage);	
    void setFcol(int x) { this->ff->setFcol(x); }	
    void calcFps();

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent (QMouseEvent* event);
    void mouseMoveEvent (QMouseEvent* event);	
    void mouseDoubleClickEvent (QMouseEvent* event);	
    void wheelEvent( QWheelEvent* );	
	void updateScalefactor();
    void ffQuit();       
    void init(); 

private:       
    int _x, _y, _maxX, _maxY, _zoom, _gx, _gy, _maxGx, _maxGy, _gs, _w, _h, _imw, _imh, _r, _g, _b;
    int xvwidth, xvheight, srcw, srch;     
    double sf;
    QTime *lastFrameTime;
    float _fps;
    QTimer *timer;
    bool _grid; 
    QString _url; 
    unsigned char* destFrame;
    unsigned int tex, qlist;
    int clickx, clicky, oldx, oldy;
    FFThreadXv *ff;  
    bool disableUpdates;    
    void xvsetup();

    XvImage * bmp;
    int xv_port;
    QX11Info qX11Info;
    Display * dpy;
    WId w;
    GC gc;
    XColor _gcol;
    
#define MAXTICKS 10
	int tickindex;
	int ticksum;
	int ticklist[MAXTICKS];
    
};

#endif
