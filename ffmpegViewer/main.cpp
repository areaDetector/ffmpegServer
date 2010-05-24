#include <QApplication>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QGridLayout>
#include <QSlider>
#include "ffmpegViewer.h"

#define clickButton( layout, name, Name, text, y ) \
    QLabel ( name ## Lbl ) ( text ); \
    (name ## Lbl).setAlignment(Qt::AlignRight | Qt::AlignVCenter); \
    (layout).addWidget(&(name ## Lbl), (y), 0);    \
    QPushButton (name ## Btn)((text)); \
    (layout).addWidget(&(name ## Btn), (y), 1); \
    view.connect(&(name ## Btn), SIGNAL(clicked()), SLOT(set ## Name()));
#define controlsButton( layout, name, Name, text, y ) \
    QLabel ( name ## Lbl ) ( text ); \
    (name ## Lbl).setAlignment(Qt::AlignRight | Qt::AlignVCenter); \
    (layout).addWidget(&(name ## Lbl), (y), 0);    \
    QPushButton (name ## Btn)((text)); \
    (layout).addWidget(&(name ## Btn), (y), 1); \
    (name ## Btn).setCheckable(true); \
    view.connect(&(name ## Btn), SIGNAL(toggled(bool)), SLOT(set ## Name(bool))); \
    (name ## Btn).connect(&view, SIGNAL(name ## Changed(bool)), SLOT(setChecked(bool)));    
#define controlsSlider( layout, name, Name, text, y, max ) \
    QLabel (name ## Lbl)((text)); \
    (name ## Lbl).setAlignment(Qt::AlignRight | Qt::AlignVCenter); \
    (layout).addWidget(&(name ## Lbl), (y), 0);    \
    QSlider (name ## Sld)(Qt::Horizontal); \
    (layout).addWidget(&(name ## Sld), (y), 1); \
    view.connect(&(name ## Sld), SIGNAL(valueChanged(int)), SLOT(set ## Name(int))); \
    (name ## Sld).connect(&view, SIGNAL(name ## Changed(int)), SLOT(setValue(int))); \
    (name ## Sld).setMaximum(max);
#define setSliderRange( name, Name ) \
    (name ## Sld).setMaximum(view.max ## Name());
//    (name ## Sld).connect(&view, SIGNAL(max ## Name ## Changed(int)), "setMaximum(int)"); 
    
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

    /* view */
    QFrame frame(&top);
    QGridLayout frameLayout;
    frame.setLayout(&frameLayout);
    QSizePolicy policy = QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    policy.setHorizontalStretch(3);
    frame.setSizePolicy(policy);
    topLayout.addWidget(&frame, 0, 0, 2, 2);
    ffmpegViewer view(argv[1], &frame);
    frame.layout()->addWidget(&view);

    /* Image controls box */
    QGroupBox icontrols(QString("Image"));
    topLayout.addWidget(&icontrols, 0, 2);
    QGridLayout icontrolsLayout;
    icontrols.setLayout(&icontrolsLayout);

    /* Image controls */
    clickButton(icontrolsLayout, reset, Reset, "Reset Image", 0)
    controlsSlider(icontrolsLayout, zoom, Zoom, "Zoom", 1, 100);
    zoomSld.setMinimum(-100);
    controlsSlider(icontrolsLayout, x, X, "X Offset", 2, 100);
    setSliderRange(x, X);
    controlsSlider(icontrolsLayout, y, Y, "Y Offset", 3, 100);
    setSliderRange(y, Y);
    
    /* Grid controls box */
    QGroupBox gcontrols(QString("Grid"));
    topLayout.addWidget(&gcontrols, 1, 2);
    QGridLayout gcontrolsLayout;
    gcontrols.setLayout(&gcontrolsLayout);

    /* Grid controls */
    controlsButton(gcontrolsLayout, grid, Grid, "Grid", 0);
    controlsSlider(gcontrolsLayout, gx, Gx, "Grid X", 1, 100);
    setSliderRange(gx, X);    
    controlsSlider(gcontrolsLayout, gy, Gy, "Grid Y", 2, 100);
    setSliderRange(gy, Y);        
    controlsSlider(gcontrolsLayout, gs, Gs, "Grid Spacing", 3, 100);
    clickButton(gcontrolsLayout, gcol, Gcol, "Grid Colour", 4)    

    /* Run the program */
    top.resize(840,480);
    top.show();
    return app.exec();
}
