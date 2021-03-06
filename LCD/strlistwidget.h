#ifndef STRLISTWIDGET_H
#define STRLISTWIDGET_H

// виджет списков строк

#include <QWidget>
#include "display.h"
#include <QListWidget>
#include <QVector>
#include "lcdphont.h"
#include <QPoint>
#include <QAction>
#include <QKeyEvent>

struct subStrActionData{
    int strNum;
    int subStrNum;
};

class StrListWidget : public QWidget
{
    Q_OBJECT

    Display& displ;
    QVector<QListWidget*> lists;
    LCDPhont* phont;

    QAction* insBefore;
    QAction* insAfter;
    QAction* delStr;
    QAction* enableDisableStr;
    QAction* copyStr;
    QAction* pasteStr;
    QAction* cutStr;
    QAction* actGroup;
    QAction* deactGroup;

    subStrActionData actData;

public:
    explicit StrListWidget(Display& d, QWidget *parent = 0);
    ~StrListWidget();
private:
    void updateList(int num);
    bool testStrNum(int strNum, int subStrNum); // проверка корректности вх. параметров
signals:
    void updFocus(void);    // необходимо передать фокус виджету дисплея
public slots:
    void strChanged(int strNum, int subStrNum);
    void strListChanged(int strNum);
private slots:
    void itemClicked(const QModelIndex& index);
    void curStrNumChanged(int strNum,int subStrNum);
    void showContextMenuForList(const QPoint &pos);

    void insertStringBefore(void);
    void insertStringAfter(void);
    void deleteString(void);
    void activateDesactString(void);
    void copyString(void);
    void pasteString(void);
    void cutString(void);
    void activateGroup(void);
    void desactivateGroup(void);

    // QWidget interface
protected:
    void keyPressEvent(QKeyEvent *event);
};

#endif // STRLISTWIDGET_H
