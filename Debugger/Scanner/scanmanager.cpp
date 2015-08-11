#include "scanmanager.h"

ScanManager::ScanManager(MemStorage *memStor, QObject *parent) : QObject(parent)
{
    scanner = new PLCScanner();
    scanner->moveToThread(&scanThread);
    connect(&scanThread,SIGNAL(finished()),scanner,SLOT(deleteLater()));
    connect(this,SIGNAL(startProcess()),scanner,SLOT(scanProcess()));
    connect(scanner,SIGNAL(updateBlock(QString,int,QByteArray)),memStor,SLOT(updateBlock(QString,int,QByteArray)));
    connect(scanner,SIGNAL(updateCorrectRequestCnt(int)),this,SLOT(updCorrAnswerCnt(int)));
    connect(scanner,SIGNAL(updateErrorRequestCnt(int)),this,SLOT(updErrAnswerCnt(int)));
    scanThread.start();
    emit startProcess();
}

void ScanManager::setScheduler(RequestScheduler *scheduler)
{
    scanner->setScheduler(scheduler);
}

ScanManager::~ScanManager()
{
    scanner->finishProcess();
    scanThread.quit();
    scanThread.wait();
}

void ScanManager::updCorrAnswerCnt(int cnt)
{
    emit updateAnswerCnt(cnt,true);
}

void ScanManager::updErrAnswerCnt(int cnt)
{
    emit updateAnswerCnt(cnt,false);
}

void ScanManager::startDebugger()
{
    scanner->startScanCmd();
}

void ScanManager::stopDebugger()
{
    scanner->stopScanCmd();
}

