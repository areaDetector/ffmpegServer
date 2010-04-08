#include <QApplication>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QGridLayout>
#include "videoWidget.h"

#define toolButton(name, text, n) QPushButton (name) ( (text) ); \
	controlsLayout.addWidget(&(name), 0, (n)); \
	(name).setAutoRepeat(true); \
	view.connect(&(name), SIGNAL(clicked()), SLOT(name()));
#define toolLabel(name, text, n) QLabel (name) ( (text) ); \
	(name).setAlignment(Qt::AlignRight | Qt::AlignVCenter); \
	controlsLayout.addWidget(&(name), 0, (n));

int main(int argc, char *argv[])
{
	/* Top level app */
    QApplication app(argc, argv);
    if (argc != 2) {
    	printf("Usage: %s <mjpg_url>\n", argv[0]);
    	return 1;
    }

	/* Create the gui */
	QWidget top;
	QGridLayout topLayout;
	top.setLayout(&topLayout);
	
	/* view in top half */
    VideoWidget view(argv[1]);
    topLayout.addWidget(&view);
     
    /* controls in bottom half */
	QGroupBox controls(QString("Controls"));
	topLayout.addWidget(&controls);    
	QGridLayout controlsLayout;
	controls.setLayout(&controlsLayout);
	
	/* buttons */
	toolButton(reset, "Reset", 0);				
	toolLabel(zoomLbl, "Zoom:", 1);
	toolButton(zoomIn, "In", 2);
	toolButton(zoomOut, "Out", 3);
	toolLabel(rotateLbl, "Rotate:", 4);	
	toolButton(rotateLeft, "Left", 5);
	toolButton(rotateRight, "Right", 6);
				    
    /* Run the program */
    top.resize(640,480);
    top.show();        
    return app.exec();
}
