#ifndef FFMPEGWIDGET_H
#define FFMPEGWIDGET_H

#include <QThread>
#include <QWheelEvent>
#include <QtDesigner/QDesignerExportWidget>
#include <QWidget>
#include <QMutex>
#include <QTime>
#include <QTimer>
#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>

/* ffmpeg includes */
extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
}

// max width of any input image
#define MAXWIDTH 4000
// max height of any input image
#define MAXHEIGHT 3000
// number of MAXWIDTH*MAXHEIGHT buffers to create
#define NBUFFERS 10
// number of frames to calc fps from
#define MAXTICKS 10
// size of URL string
#define MAXSTRING 1024

class FFBuffer
{
public:
    FFBuffer ();
    ~FFBuffer ();
    QMutex *mutex;
    unsigned char *mem;
    AVFrame *pFrame;
    PixelFormat pix_fmt;
    int width;
    int height;
};

class FFThread : public QThread
{
    Q_OBJECT

public:
    FFThread (const QString &url, PixelFormat dest_format, int maxW, int maxH, QWidget* parent);
    ~FFThread ();
    void run();
    FFBuffer * findFreeBuffer();
    FFBuffer * formatFrame(FFBuffer *src, int width, int height, PixelFormat pix_fmt, struct SwsContext *ctx);
    FFBuffer * falseFrame(FFBuffer *src, int width, int height, PixelFormat pix_fmt);

    int fcol() { return _fcol; }

public slots:
    void stopGracefully() { stopping = 1; }
    void setFcol(int fcol) { _fcol = fcol; }

signals:
    void updateSignal(FFBuffer * buf, bool firstImage);

private:
    char url[MAXSTRING];
    int stopping;
    int _fcol;
    PixelFormat dest_format;
    struct SwsContext *ctx;
    int maxW, maxH;
};

class QDESIGNER_WIDGET_EXPORT ffmpegWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY( int x READ x WRITE setX)             // x offset in image pixels
    Q_PROPERTY( int y READ y WRITE setY)             // y offset in image pixels
    Q_PROPERTY( int zoom READ zoom WRITE setZoom)    // zoom level
    Q_PROPERTY( int gx READ gx WRITE setGx)          // grid x in image pixels
    Q_PROPERTY( int gy READ gy WRITE setGy)          // grid y in image pixels
    Q_PROPERTY( bool grid READ grid WRITE setGrid)   // grid on or off
    Q_PROPERTY( QColor gcol READ gcol WRITE setGcol) // grid colour
    Q_PROPERTY( int fcol READ fcol WRITE setFcol)    // false colour
    Q_PROPERTY( QString url READ url WRITE setUrl)   // ffmpeg url


public:
    /* Constructor and destructors */
    ffmpegWidget (QWidget* parent = 0);
    ~ffmpegWidget ();

    /* Getters: read/write variables */
    int x() const           { return _x; }      // x offset in image pixels
    int y() const           { return _y; }      // y offset in image pixels
    int zoom() const        { return _zoom; }   // zoom level
    int gx() const          { return _gx; }     // grid x in image pixels
    int gy() const          { return _gy; }     // grid y in image pixels
    bool grid() const       { return _grid; }   // grid on or off
    QColor gcol() const     { return _gcol; }   // grid colour
    int fcol() const        { return _fcol; }   // false colour
    QString url() const     { return _url; }    // ffmpeg url

    /* Getters: read only */
    int maxX() const        { return _maxX; }   // Max x offset in image pixels
    int maxY() const        { return _maxY; }   // Max y offset in image pixels
    int maxGx() const       { return _maxGx; }  // Max grid x offset in image pixels
    int maxGy() const       { return _maxGy; }  // Max grid y offset in image pixels
    int imW() const         { return _imW; }    // Image width in image pixels
    int imH() const         { return _imH; }    // Image height in image pixels
    int visW() const        { return _visW; }   // Image width currently visible in image pixels
    int visH() const        { return _visH; }   // Image height currently visible in image pixels
    int scImW() const       { return _scImW; }  // Image width in viewport scaled pixels
    int scImH() const       { return _scImH; }  // Image height in viewport scaled pixels
    int scVisW() const      { return _scVisW; } // Image width visible in viewport scaled pixels
    int scVisH() const      { return _scVisH; } // Image height visible in viewport scaled pixels
    double fps() const      { return _fps; }    // Frames per second displayed

signals:
    /* Signals: read/write variables */
    void xChanged(int);                         // x offset in image pixels
    void yChanged(int);                         // y offset in image pixels
    void zoomChanged(int);                      // zoom level
    void gxChanged(int);                        // grid x in image pixels
    void gyChanged(int);                        // grid y in image pixels
    void gridChanged(bool);                     // grid on or off
    void gcolChanged(QColor);                   // grid colour
    void fcolChanged(int);                      // false colour
    void urlChanged(QString);                   // ffmpeg url

    /* Signals: read only */
    void maxXChanged(int);                      // Max x offset in image pixels
    void maxYChanged(int);                      // Max y offset in image pixels
    void maxGxChanged(int);                     // Max grid x offset in image pixels
    void maxGyChanged(int);                     // Max grid y offset in image pixels
    void imWChanged(int) ;                      // Image width in image pixels
    void imHChanged(int);                       // Image height in image pixels
    void visWChanged(int);                      // Image width currently visible in image pixels
    void visHChanged(int);                      // Image height currently visible in image pixels
    void scImWChanged(int);                     // Image width in viewport scaled pixels
    void scImHChanged(int);                     // Image height in viewport scaled pixels
    void scVisWChanged(int);                    // Image width visible in viewport scaled pixels
    void scVisHChanged(int);                    // Image height visible in viewport scaled pixels
    void fpsChanged(double);                    // Frames per second displayed

    /* Signals: other */
    void visWChanged(QString);
    void visHChanged(QString);
    void fpsChanged(QString);
    void aboutToQuit();

public slots:
    /* Slots: read/write variables */
    void setX(int);                         // x offset in image pixels
    void setY(int);                         // y offset in image pixels
    void setZoom(int);                      // zoom level
    void setGx(int);                        // grid x in image pixels
    void setGy(int);                        // grid y in image pixels
    void setGrid(bool);                     // grid on or off
    void setGcol(QColor);                   // grid colour
    void setFcol(int);                      // false colour
    void setUrl(QString);                   // ffmpeg url

    /* Slots: others */
    void setGcol();
    void setReset();
    void calcFps();
    void updateImage(FFBuffer *buf, bool firstImage);

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent (QMouseEvent* event);
    void mouseMoveEvent (QMouseEvent* event);
    void mouseDoubleClickEvent (QMouseEvent* event);
    void wheelEvent( QWheelEvent* );
    void updateScalefactor();
    void ffQuit();
    // xv stuff
    void xvSetup();
    int xv_port;
    int xv_format;
    XvImage * xv_image;
    Display * dpy;
    WId w;
    GC gc;
    // other
    double sf;
    FFBuffer *buf;
    QTime *lastFrameTime;
    QTimer *timer;
    int widgetW, widgetH;
    int clickx, clicky, oldx, oldy;
    FFThread *ff;
    bool disableUpdates;
    PixelFormat ff_fmt;
    // fps calculation
    int tickindex;
    int ticksum;
    int ticklist[MAXTICKS];
    int maxW, maxH;

private:
    /* Private variables, read/write */
    int _x;       // x offset in image pixels
    int _y;       // y offset in image pixels
    int _zoom;    // zoom level
    int _gx;      // grid x in image pixels
    int _gy;      // grid y in image pixels
    bool _grid;   // grid on or off
    QColor _gcol; // grid colour
    int _fcol;    // false colour
    QString _url; // ffmpeg url

    /* Private variables: read only */
    int _maxX;    // Max x offset in image pixels
    int _maxY;    // Max y offset in image pixels
    int _maxGx;   // Max grid x offset in image pixels
    int _maxGy;   // Max grid y offset in image pixels
    int _imW;     // Image width in image pixels
    int _imH;     // Image height in image pixels
    int _visW;    // Image width currently visible in image pixels
    int _visH;    // Image height currently visible in image pixels
    int _scImW;   // Image width in viewport scaled pixels
    int _scImH;   // Image height in viewport scaled pixels
    int _scVisW;  // Image width visible in viewport scaled pixels
    int _scVisH;  // Image height visible in viewport scaled pixels
    double _fps;  // Frames per second displayed
};

#endif
