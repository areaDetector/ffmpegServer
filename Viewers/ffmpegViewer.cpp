#include "ui_ffmpegViewer.h"
#include "caValueMonitor.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    /* Top level app */
    QApplication app(argc, argv);

    if (argc != 2 && argc != 3) {
        printf("Usage: %s <mjpg_url> [<CA prefix for grid>]\n", argv[0]);
        return 1;
    }

    /* Make a widget to turn into the form */
    QMainWindow *top = new QMainWindow;
    Ui::ffmpegViewer ui;
    ui.setupUi(top);
    ui.video->setUrl(argv[1]);

    /* Connect it to CA */
    if (argc == 3) {
        caValueMonitor *mon = new caValueMonitor("GC1020C:MJPG", top);
        QObject::connect( mon, SIGNAL(gxChanged(int)),
                          ui.video, SLOT(setGx(int)) );
        QObject::connect( ui.video, SIGNAL(gxChanged(int)),
                          mon, SLOT(setGx(int)) );
        QObject::connect( mon, SIGNAL(gyChanged(int)),
                          ui.video, SLOT(setGy(int)) );
        QObject::connect( ui.video, SIGNAL(gyChanged(int)),
                          mon, SLOT(setGy(int)) );
        QObject::connect( mon, SIGNAL(gridChanged(bool)),
                          ui.video, SLOT(setGrid(bool)) );
        QObject::connect( ui.video, SIGNAL(gridChanged(bool)),
                          mon, SLOT(setGrid(bool)) );
        QObject::connect( mon, SIGNAL(gcolChanged(QColor)),
                          ui.video, SLOT(setGcol(QColor)) );
        QObject::connect( ui.video, SIGNAL(gcolChanged(QColor)),
                          mon, SLOT(setGcol(QColor)) );
        mon->start();
    }

    /* Show it */
    top->show();
    return app.exec();
}
