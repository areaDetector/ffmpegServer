#include "ui_ffmpegViewer.h"
#include "caValueMonitor.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    /* Top level app */
    QApplication app(argc, argv);

    /* Parse the arguments */
    QString url, prefix;
    char * usage = "Usage: %s [options] <mjpg_url> [<CA prefix for grid>]\n\n" \
        "\t-h\tShow this help message and quit\n" \
        "\t-f\tFallback mode, don't try to use xvideo\n";
    for (int i = 1; i < app.arguments().size(); i++) {
        if (app.arguments().at(i) == "-f") {
            // fallback mode
            fallback = 1;
        } else if (app.arguments().at(i) == "-h") {
            // asked for help
            printf(usage, argv[0]);
            return 1;              
        } else if (url.isNull()) {
            // first positional arg is mjpg_url
            url = app.arguments().at(i);
        } else if (prefix.isNull()) {
            // second positional arg is CA prefix
            prefix = app.arguments().at(i);
        } else {
            // asked for help or too many args 
            printf(usage, argv[0]);
            return 1;
        }
    }
    
    /* If we didn't specify url return usage */
    if (url.isNull()) {
        printf(usage, argv[0]);
        return 1;
    }

    /* Make a widget to turn into the form */
    QMainWindow *top = new QMainWindow;
    Ui::ffmpegViewer ui;
    ui.setupUi(top);
    ui.video->setUrl(url);
    top->setWindowTitle(QString("ffmpegViewer: %1").arg(url));

    /* Connect it to CA */
    if (!prefix.isNull()) {
        caValueMonitor *mon = new caValueMonitor(prefix, top);
        QObject::connect( mon, SIGNAL(gxChanged(int)),
                          ui.video, SLOT(setGx(int)) );
        QObject::connect( ui.video, SIGNAL(gxChanged(int)),
                          mon, SLOT(setGx(int)) );
        QObject::connect( mon, SIGNAL(gyChanged(int)),
                          ui.video, SLOT(setGy(int)) );
        QObject::connect( ui.video, SIGNAL(gyChanged(int)),
                          mon, SLOT(setGy(int)) );
        QObject::connect( mon, SIGNAL(gsChanged(int)),
                          ui.video, SLOT(setGs(int)) );
        QObject::connect( ui.video, SIGNAL(gsChanged(int)),
                          mon, SLOT(setGs(int)) );                          
        QObject::connect( mon, SIGNAL(gridChanged(bool)),
                          ui.video, SLOT(setGrid(bool)) );
        QObject::connect( ui.video, SIGNAL(gridChanged(bool)),
                          mon, SLOT(setGrid(bool)) );
        QObject::connect( mon, SIGNAL(gcolChanged(QColor)),
                          ui.video, SLOT(setGcol(QColor)) );
        QObject::connect( ui.video, SIGNAL(gcolChanged(QColor)),
                          mon, SLOT(setGcol(QColor)) );
        mon->start();
    } else {
		/* Set the grid */
		ui.video->setGx(100);
		ui.video->setGy(100);	
		ui.video->setGs(10);		
	}    
    
    /* Show it */
    top->show();
    return app.exec();
}
