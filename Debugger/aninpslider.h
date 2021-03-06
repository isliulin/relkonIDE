#ifndef ANINPSLIDER_H
#define ANINPSLIDER_H

// потомок виджета QSlider
// реализует возможность изменять значение ползунка клавишами "влево" и "вправо"

#include <QObject>
#include <QSlider>

class AnInpSlider : public QSlider
{
    Q_OBJECT
    bool eightBit;
public:
    AnInpSlider(QWidget *parent = 0);

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
public:
    void setEightBit(bool value);
};

#endif // ANINPSLIDER_H
