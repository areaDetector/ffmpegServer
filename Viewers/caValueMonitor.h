#ifndef CAVALUEMONITOR_H
#define CAVALUEMONITOR_H

#include <QThread>
#include <QTimer>
#include <QColor>
#include <cadef.h>

class caValueMonitor : public QObject
{
    Q_OBJECT

public:
    caValueMonitor (const QString &prefix, QWidget* parent);
    ~caValueMonitor ();
    void start();
    void eventCallback(struct event_handler_args args);

public slots:
    void setGx(int);
    void setGy(int);
    void setGcol(QColor);
    void setGrid(bool);
    void setGs(int);    
    void doWrite();

signals:
    void gxChanged(int);
    void gyChanged(int);
    void gcolChanged(QColor);
    void gridChanged(bool);
    void gsChanged(int);

private:
    chid gxChid, gyChid, gcolChid, gridChid, gsChid;
    long gxLast, gyLast, gcolLast, gridLast, gsLast;
    long gxCurrent, gyCurrent, gcolCurrent, gridCurrent, gsCurrent;
    void *sendBuf;
    QString prefix;
    QTimer *timer;
};

#endif
