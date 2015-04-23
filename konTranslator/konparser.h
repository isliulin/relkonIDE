#ifndef KONPARSER_H
#define KONPARSER_H

#include <QStringList>
#include "textblock.h"
#include "konprocess.h"
#include "abstractchfilesfactory.h"

class KonParser
{
    const QString sourceKonFileName="input.kon";
    QStringList konSource;
    TextBlock varBlock;
    TextBlock initBlock;
    QVector<KonProcess> prBlock;
    AbstractCHFilesFactory* factory;
    QStringList parsingErrors;

    void removeComments(void);
    void addStringNum(void);
    int readSourceFile(void);
    int createVarBlock(void);
    int createInitBlock(void);
    int createProcessBlock(void);

public:
    KonParser();
    void setCHFactory(AbstractCHFilesFactory* factory);
    void parse(void);
    const QStringList &getErrors(void) const;
};

#endif // KONPARSER_H
