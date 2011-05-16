#ifndef SSPINBOX_H
#define SSPINBOX_H

#include <QSpinBox>

class SSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    SSpinBox(QWidget *parent = 0);

public slots:
    void setMaximumSlot(int max) {setMaximum(max);}	
};

#endif
