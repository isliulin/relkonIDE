#ifndef DISPLAYWIDGET_H
#define DISPLAYWIDGET_H

// виджет имитации дисплея с возможностью ввода данных

#include <QWidget>
#include "lcdphont.h"
#include "display.h"

class DisplayWidget : public QWidget
{
    Q_OBJECT
    LCDPhont* phont;
    static const int strCount = 4;
    static const int spaceBetwSymb = 5;
    int symbInStrCount;

    Display& displ;
public:
    explicit DisplayWidget(Display& d, QWidget *parent = 0);
    const LCDPhont&  getPhont(void) const {return *phont;}
    ~DisplayWidget();

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void paintEvent(QPaintEvent *event);

};

#endif // DISPLAYWIDGET_H
