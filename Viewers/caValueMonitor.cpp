#include "caValueMonitor.h"
#include <QWidget>

void connectCallbackC(struct connection_handler_args args) {
    printf("Connected: %p, %ld\n", args.chid, args.op);
}

void eventCallbackC(struct event_handler_args args) {
    caValueMonitor *ptr = (caValueMonitor *) args.usr;
    ptr->eventCallback(args);
}

caValueMonitor::caValueMonitor(const QString &prefix, QWidget* parent)
    : QObject (parent)
{
    this->prefix = QString(prefix);
    this->sendBuf = calloc(1, dbr_size_n(DBR_LONG, 1));
}

caValueMonitor::~caValueMonitor() {
    free(sendBuf);
}

void caValueMonitor::start() {
    ca_context_create(ca_enable_preemptive_callback);
    ca_create_channel((prefix + QString(":GX")).toAscii().data(), NULL, NULL, 0, &this->gxChid);
    ca_create_channel((prefix + QString(":GY")).toAscii().data(), NULL, NULL, 0, &this->gyChid);
    ca_create_channel((prefix + QString(":GCOL")).toAscii().data(), NULL, NULL, 0, &this->gcolChid);
    ca_create_channel((prefix + QString(":GRID")).toAscii().data(), NULL, NULL, 0, &this->gridChid);
    ca_create_subscription(DBR_LONG, 1, this->gxChid, DBE_VALUE, eventCallbackC, (void*)this, NULL);
    ca_create_subscription(DBR_LONG, 1, this->gyChid, DBE_VALUE, eventCallbackC, (void*)this, NULL);
    ca_create_subscription(DBR_LONG, 1, this->gcolChid, DBE_VALUE, eventCallbackC, (void*)this, NULL);
    ca_create_subscription(DBR_LONG, 1, this->gridChid, DBE_VALUE, eventCallbackC, (void*)this, NULL);
    ca_pend_io(3);
}

void caValueMonitor::eventCallback(struct event_handler_args args) {
    if(args.status != ECA_NORMAL) {
        fprintf(stderr, "error: %s\n", ca_message(args.status));
        return;
    }
    unsigned int value = *(unsigned int *)args.dbr;
    if (args.chid == gxChid) {
        emit gxChanged(value);
    } else if (args.chid == this->gyChid) {
        emit gyChanged(value);
    } else if (args.chid == this->gridChid) {
        emit gridChanged((bool) value);
    } else if (args.chid == this->gcolChid) {
        emit gcolChanged(QColor((QRgb) value));
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
    *(long*)this->sendBuf = gcol.rgb();
    ca_array_put(DBR_LONG, 1, this->gcolChid, sendBuf);
    ca_pend_io(3);
}

void caValueMonitor::setGrid(bool grid) {
    *(long*)this->sendBuf = grid;
    ca_array_put(DBR_LONG, 1, this->gridChid, sendBuf);
    ca_pend_io(3);
}

