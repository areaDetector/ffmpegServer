#include "ui_webcam4.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    /* Top level app */
    QApplication app(argc, argv);

    /* Parse the arguments */
    QString topleft, bottomleft, topright, bottomright, leftlabel, rightlabel;
    char * usage = "Usage: %s [options] <topleft> <bottomleft> <topright> <bottomright> [<leftlabel=Optics Hutch>] [<rightlabel=Experimental Hutch>]\n\n" \
        "Where topleft .. bottom right are urls for ffmpeg streams\n" \
        "E.g. http://i11-webcam2.diamond.ac.uk/mjpg/video.mjpg\n\n" \
        "Options:\n" \
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
        } else if (topleft.isNull()) {
            // 1st positional arg is topleft url
            topleft = app.arguments().at(i);
        } else if (bottomleft.isNull()) {
            // 2nd positional arg is bottomleft url
            bottomleft = app.arguments().at(i);
        } else if (topright.isNull()) {
            // 3rd positional arg is topright url
            topright = app.arguments().at(i);
        } else if (bottomright.isNull()) {
            // 4th positional arg is bottomright url
            bottomright = app.arguments().at(i);
        } else if (leftlabel.isNull()) {
            // 5th positional arg is leftlabel text
            leftlabel = app.arguments().at(i);
        } else if (rightlabel.isNull()) {
            // 6th positional arg is rightlabel text
            rightlabel = app.arguments().at(i);
        } else {
            // too many args 
            printf(usage, argv[0]);
            return 1;
        }
    }
    
    /* If we didn't specify enough urls return usage */
    if (topleft.isNull() || bottomleft.isNull() || topright.isNull() || bottomright.isNull()) {
        printf(usage, argv[0]);
        return 1;
    }

    /* Make a widget to turn into the form */
    QWidget *top = new QWidget;
    Ui::webcam4 ui;
    ui.setupUi(top);
    ui.topleft->setUrl(topleft);
    ui.bottomleft->setUrl(bottomleft);
    ui.topright->setUrl(topright);
    ui.bottomright->setUrl(bottomright);
    if (!leftlabel.isNull()) ui.leftlabel->setText(leftlabel);            
    if (!rightlabel.isNull()) ui.rightlabel->setText(rightlabel);
    //top->setWindowTitle(QString("ffmpegViewer: %1").arg(url));
   
    /* Show it */
    top->show();
    return app.exec();
}
