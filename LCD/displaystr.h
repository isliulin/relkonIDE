#ifndef DISPLAYSTR_H
#define DISPLAYSTR_H

// класс строки дисплея

#include <QByteArray>
#include "varpattern.h"
#include <QVector>
#include <QMutexLocker>
#include "pultvardefinition.h"

class DisplayStr
{
    QVector<PultVarDefinition*> vList;  // список переменных
    static const int length=20; // длина строки
    static const quint8 spaceCode = 0x20;
    QByteArray data;
    static bool replaceMode;
    mutable QMutex mutex;   // для поддержки работы в многопоточном приложении
    bool active;  // активное/пассивное состояние строки

    static bool checkPosition(int pos);  // проверка корректности значения pos
    int getFreeSpace(void) const;   // возвращает количество пробелов в конце строки

public:
    DisplayStr();
    DisplayStr(const DisplayStr &s);
    DisplayStr& operator=(const DisplayStr &s);
    static int getLength(void) {return length;}
    ~DisplayStr();
    static bool getReplaceMode();
    int getSymbol(int pos) const;
    const QByteArray getString(void) const {QMutexLocker locker(&mutex);return data;}
    bool insertSymbol(int pos, quint8 code);
    int deleteSymbol(int pos);
    static void setReplaceMode(bool value);
    bool addVar(const PultVarDefinition& vDef);
    bool updVar(const PultVarDefinition& vDef);
    int getVarsCount(void) const {return vList.count();}
    bool getVar(int num, PultVarDefinition &vd) const;
    void updVarDefinition(int num, PultVarDefinition &vd);
    bool getVarInPos(int pos, PultVarDefinition &vd) const;
    QString getVarID(int pos) const;
    QString getVarPatern(int pos) const;
    bool isActive(void) const {return active;}
    void setActive(bool value) {active=value;}
    bool isVarHere(int pos) const;
    bool isThisABeginningOfVar(int pos) const;
};

#endif // DISPLAYSTR_H
