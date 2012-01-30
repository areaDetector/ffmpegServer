#ifndef CAVALUEMONITOR_H
#define CAVALUEMONITOR_H

#include <QThread>
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

signals:
	void gxChanged(int);
	void gyChanged(int);
	void gcolChanged(QColor);
	void gridChanged(bool);									

private:
	chid gxChid, gyChid, gcolChid, gridChid;
	void *sendBuf;
	QString prefix;
};

#endif
