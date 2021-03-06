#ifndef REQUESTSCHEDULER_H
#define REQUESTSCHEDULER_H

// планировщик запросов

#include <QObject>
#include "Protocols/rkcommand.h"
#include "Protocols/request.h"
#include <Qvector>
#include <QMutex>
#include "VarDef/varitem.h"
#include <QHash>
#include <QString>
#include <QBitArray>

struct CmdData{
    CommandInterface* cmd;
    Request* req;
};

class RequestScheduler : public QObject
{
    QMutex mutex;
    QVector<CmdData*> cmdQueue; // основная очередь команд
    QVector<CmdData*> binQueue; // хранилище для отложенного удаления
    QVector<CmdData*> wrQueue;  // команды на запись
    bool removeWr;       // флаг необходимости удалить команду записи из очереди
    int i;  // текущий индекс
    QHash<QString,QBitArray> devMap;    // тип памяти - битовые поля (1 - опраш./0 - не опраш.)
    Q_OBJECT

    static const int maxHole = 10;
    static const int maxLength = 64;

    void clearBin(void);    // очистить хранилище для отложенного удаления
    CommandInterface* getRdCmdByMemType(const QString &memType);
    CommandInterface* getWrCmdByMemType(const QString &memType);
    void scanMap(const QString &memType);
public:
    explicit RequestScheduler(QObject *parent = 0);
    CommandInterface* getCmd(void);  // возвращает команду для выполнения
    Request getReq(void);   // данные запроса для команды
    void moveToNext();  // переместиться к следующей команде в цикле
    void addReadOperation(VarItem v);   // добавить операцию чтения переменной
    void removeReadOperation(VarItem v);    // удалить чтение области памяти переменной
    void addWriteOperation(VarItem v);  // запрос на запись переменной
    void schedule();    // сформировать очередь запросов
    void clear();
    ~RequestScheduler();

signals:

public slots:

};

#endif // REQUESTSCHEDULER_H
