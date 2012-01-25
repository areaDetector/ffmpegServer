#define USE_XV_PLAYER

#include <QApplication>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QGridLayout>

#ifdef USE_XV_PLAYER
#include "ffmpegWidget.h"
#else
#include "ffmpegViewer.h"
#endif

#include "SSpinBox.h"

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
    SSpinBox (name ## Sld); \
    (layout).addWidget(&(name ## Sld), (y), 1); \
    view.connect(&(name ## Sld), SIGNAL(valueChanged(int)), SLOT(set ## Name(int))); \
    (name ## Sld).connect(&view, SIGNAL(name ## Changed(int)), SLOT(setValue(int))); \
    (name ## Sld).setMaximum(max);
#define setSliderRange( name, Name ) \
    (name ## Sld).setMaximum(view.max ## Name()); \
    (name ## Sld).connect(&view, SIGNAL(max ## Name ## Changed(int)), SLOT(setMaximumSlot(int))); 

    
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
    QString windowTitle(argv[1]);
    QStringList wlist = windowTitle.split("/");
    windowTitle = QString("ffmpegViewer: ") + wlist[wlist.size() - 1];    
    top.setWindowTitle(windowTitle);

    /* view */
    QFrame frame(&top);
    QGridLayout frameLayout;
    frame.setLayout(&frameLayout);
    QSizePolicy policy = QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    policy.setHorizontalStretch(3);
    frame.setSizePolicy(policy);
    topLayout.addWidget(&frame, 0, 0, 2, 2);
#ifdef USE_XV_PLAYER
    ffmpegWidget view(argv[1], &frame);
#else
    ffmpegViewer view(argv[1], &frame);
#endif
    frame.layout()->addWidget(&view);

    /* Image controls box */
    QGroupBox icontrols(QString("Image"));
    topLayout.addWidget(&icontrols, 0, 2);
    QGridLayout icontrolsLayout;
    icontrols.setLayout(&icontrolsLayout);
    
    /* Image controls */
    clickButton(icontrolsLayout, reset, Reset, "Reset Image", 0)
    controlsSlider(icontrolsLayout, zoom, Zoom, "Zoom", 1, 30);
    zoomSld.setMinimum(0);
    controlsSlider(icontrolsLayout, x, X, "X Offset", 2, 100);
    setSliderRange(x, X);
    controlsSlider(icontrolsLayout, y, Y, "Y Offset", 3, 100);
    setSliderRange(y, Y);
    QLabel falseColLbl("False Colour Map");
    falseColLbl.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    icontrolsLayout.addWidget(&falseColLbl, 4, 0);
    QComboBox falseColBtn;
    icontrolsLayout.addWidget(&falseColBtn, 4, 1);
    falseColBtn.addItem("None");
    falseColBtn.addItem("Rainbow");
    falseColBtn.addItem("Iron");        
    view.connect(&falseColBtn, SIGNAL(currentIndexChanged(int)), SLOT(setFcol(int))); \
    QLabel fpsLbl("fps");
    fpsLbl.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    icontrolsLayout.addWidget(&(fpsLbl), 5, 0);
    QLineEdit fpsVal("0.0");
    fpsVal.setReadOnly(true);
    icontrolsLayout.addWidget(&(fpsVal), 5, 1);
    fpsVal.connect(&(view), SIGNAL(fpsChanged(const QString &)), SLOT(setText(const QString &)));    
        
    /* Grid controls box */
    QGroupBox gcontrols(QString("Grid"));
    topLayout.addWidget(&gcontrols, 1, 2);
    QGridLayout gcontrolsLayout;
    gcontrols.setLayout(&gcontrolsLayout);

    /* Grid controls */
    controlsButton(gcontrolsLayout, grid, Grid, "Grid", 0);
    controlsSlider(gcontrolsLayout, gx, Gx, "Grid X", 1, 100);
    setSliderRange(gx, Gx);    
    controlsSlider(gcontrolsLayout, gy, Gy, "Grid Y", 2, 100);
    setSliderRange(gy, Gy);        
    controlsSlider(gcontrolsLayout, gs, Gs, "Grid Spacing", 3, 100);
    gsSld.setMinimum(2);    
    gsSld.setValue(20);        
    clickButton(gcontrolsLayout, gcol, Gcol, "Grid Colour", 4)    

    /* Run the program */
    top.resize(1266,808);
    top.show();
    return app.exec();
}
