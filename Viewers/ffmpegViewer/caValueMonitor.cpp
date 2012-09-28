#include "caValueMonitor.h"
#include <QWidget>
#include <stdio.h>

void eventCallbackC(struct event_handler_args args) {
    caValueMonitor *ptr = (caValueMonitor *) args.usr;
    ptr->eventCallback(args);
}

caValueMonitor::caValueMonitor(const QString &prefix, QWidget* parent)
    : QObject (parent)
{
    this->prefix = QString(prefix);
    this->sendBuf = calloc(1, dbr_size_n(DBR_LONG, 1));
    this->timer = new QTimer();
    this->gxLast = -1;
    this->gyLast = -1;
    this->gcolLast = -1;
    this->gridLast = -1;                            
    this->gsLast = -1;    
    this->gxCurrent = -1;
    this->gyCurrent = -1;
    this->gcolCurrent = -1;
    this->gridCurrent = -1;        
    this->gsCurrent = -1;    
    QObject::connect( this->timer, SIGNAL(timeout()),
                      this, SLOT(doWrite()) );   
    this->timer->start(100);                         
}

caValueMonitor::~caValueMonitor() {
    free(sendBuf);
}

void caValueMonitor::start() {
    ca_context_create(ca_enable_preemptive_callback);
    ca_create_channel((prefix + QString("GX")).toAscii().data(), NULL, NULL, 0, &this->gxChid);
    ca_create_channel((prefix + QString("GY")).toAscii().data(), NULL, NULL, 0, &this->gyChid);
    ca_create_channel((prefix + QString("GCOL")).toAscii().data(), NULL, NULL, 0, &this->gcolChid);
    ca_create_channel((prefix + QString("GRID")).toAscii().data(), NULL, NULL, 0, &this->gridChid);
    ca_create_channel((prefix + QString("GS")).toAscii().data(), NULL, NULL, 0, &this->gsChid);    
    ca_create_subscription(DBR_LONG, 1, this->gxChid, DBE_VALUE, eventCallbackC, (void*)this, NULL);
    ca_create_subscription(DBR_LONG, 1, this->gyChid, DBE_VALUE, eventCallbackC, (void*)this, NULL);
    ca_create_subscription(DBR_LONG, 1, this->gcolChid, DBE_VALUE, eventCallbackC, (void*)this, NULL);
    ca_create_subscription(DBR_LONG, 1, this->gridChid, DBE_VALUE, eventCallbackC, (void*)this, NULL);
    ca_create_subscription(DBR_LONG, 1, this->gsChid, DBE_VALUE, eventCallbackC, (void*)this, NULL);    
    ca_pend_io(3);
}

void caValueMonitor::doWrite() {
	if (this->gxLast != this->gxCurrent) {
    	this->gxLast = this->gxCurrent;	
        emit gxChanged(this->gxLast);
    } else if (this->gyLast != this->gyCurrent) {
    	this->gyLast = this->gyCurrent;	
        emit gyChanged(this->gyLast);
    } else if (this->gcolLast != this->gcolCurrent) {
    	this->gcolLast = this->gcolCurrent;	
        emit gcolChanged(QColor((QRgb) (0xFF000000 + this->gcolLast)));
    } else if (this->gridLast != this->gridCurrent) {
    	this->gridLast = this->gridCurrent;	
        emit gridChanged((bool) this->gridLast);
    } else if (this->gsLast != this->gsCurrent) {
    	this->gsLast = this->gsCurrent;	
        emit gsChanged(this->gsLast);
    }
}	

void caValueMonitor::eventCallback(struct event_handler_args args) {
    if(args.status != ECA_NORMAL) {
        fprintf(stderr, "error: %s\n", ca_message(args.status));
        return;
    }
    unsigned int value = *(unsigned int *)args.dbr;
    if (args.chid == this->gxChid) {
    	this->gxCurrent = value;
    } else if (args.chid == this->gyChid) {
    	this->gyCurrent = value;
    } else if (args.chid == this->gridChid) {
    	this->gridCurrent = value;
    } else if (args.chid == this->gcolChid) {
    	this->gcolCurrent = value;
    } else if (args.chid == this->gsChid) {
    	this->gsCurrent = value;
    }
}

void caValueMonitor::setGx(int gx) {
    *(long*)this->sendBuf = gx;
    ca_array_put(DBR_LONG, 1, this->gxChid, sendBuf);
    ca_pend_io(3);
}

void caValueMonitor::setGy(int gy) {
    *(long*)this->sendBuf = gy;
    ca_array_put(DBR_LONG, 1, this->gyChid, sendBuf);
    ca_pend_io(3);
}

void caValueMonitor::setGcol(QColor gcol) {
    *(long*)this->sendBuf = gcol.rgb() - 0xFF000000;
    ca_array_put(DBR_LONG, 1, this->gcolChid, sendBuf);
    ca_pend_io(3);
}

void caValueMonitor::setGrid(bool grid) {
    *(long*)this->sendBuf = grid;
    ca_array_put(DBR_LONG, 1, this->gridChid, sendBuf);
    ca_pend_io(3);
}

void caValueMonitor::setGs(int gs) {
    *(long*)this->sendBuf = gs;
    ca_array_put(DBR_LONG, 1, this->gsChid, sendBuf);
    ca_pend_io(3);
}
