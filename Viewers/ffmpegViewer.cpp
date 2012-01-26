#include "ui_ffmpegViewer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    /* Top level app */
    QApplication app(argc, argv);
    if (argc != 2) {
        printf("Usage: %s <mjpg_url>\n", argv[0]);
        return 1;
    }

	/* Make a widget to turn into the form */
    QMainWindow *top = new QMainWindow;
    Ui::ffmpegViewer ui;
    ui.setupUi(top);
    ui.video->setUrl(argv[1]);
    ui.video->ffInit();
    top->show();
    return app.exec();
}
