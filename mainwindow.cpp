#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "highlighter.h"

#include <QLabel>
#include <QFileDialog>
#include <QTextStream>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QSpacerItem>
#include <QDateTime>
#include <QComboBox>
#include <QSettings>
#include <QFileInfo>
#include <QMessageBox>

#include "prbuilder.h"
#include "settingsform.h"
#include "AutoSearch/scangui.h"
#include "Loader/bootmodesetter.h"
#include "Loader/ymodemthread.h"
#include "AutoSearch/detectedcontroller.h"
#include <QSerialPort>
#include "RCompiler/rcompiler.h"

#include "Debugger/debuggerform.h"



QStringList MainWindow::getPrevProjects()
{
    QStringList res;
    QSettings settings("Kontel","RIDE");
    res = settings.value("/Settings/PrevProjects",QStringList()).toStringList();
    res.removeDuplicates();
    if(res.count()>maxAmountOfPrevProjects) res = res.mid(0,maxAmountOfPrevProjects);
    settings.setValue("/Settings/PrevProjects",res);
    return res;
}

void MainWindow::updatePrevProjects(const QStringList &prNames)
{
    QSettings settings("Kontel","RIDE");
    settings.setValue("/Settings/PrevProjects",prNames);
    QStringList resList = getPrevProjects();
    ui->menu->clear();

    ui->menu->addAction(newAct);
    ui->menu->addAction(openAct);
    ui->menu->addAction(saveAct);
    ui->menu->addAction(saveAsAct);
    QMenu* recPr = new QMenu("Недавние проекты");
    ui->menu->addMenu(recPr);
    foreach(QString name, resList) {
        //ui->menu->addAction(name,this,SLOT(openPrevProject()));
        recPr->addAction(name,this,SLOT(openPrevProject()));
    }
}

int MainWindow::openFileByName(const QString &fName)
{
    activateInfoPanel();
    addMessageToInfoList(QDateTime::currentDateTime().time().toString() + " :Открытие файла " + fName);

    QFile file(fName);
    if (file.open(QIODevice::ReadOnly)) {

        QFileInfo fInfo(fName);
        prDirPath = fInfo.absolutePath();
        prFileName = fInfo.fileName();

        QStringList prevPr = getPrevProjects();
        prevPr.insert(0,fName);
        updatePrevProjects(prevPr);

        editor->clear();
        QTextStream in(&file);
        in.setCodec("Windows-1251");
        editor->setDisabled(true);
        editor->appendPlainText(in.readAll());
        editor->setEnabled(true);

        QTextCursor tmpCursor = editor->textCursor();
        tmpCursor.setVisualNavigation(true);
        tmpCursor.setPosition(editor->document()->firstBlock().position());
        editor->setTextCursor(tmpCursor);
        file.close();
        addMessageToInfoList(QDateTime::currentDateTime().time().toString() + " :Файл успешно открыт");
        setWindowTitle(wTitle + " - " + fName);
        RCompiler::setInpDirName(fInfo.dir().path());
        RCompiler::setInpKonFileName(fInfo.fileName());
        if(settings!=nullptr) {
            settings->clearSettings();
            settings->setKonFileName(fName);
            settings->openSettings();
        }
        editor->document()->clearUndoRedoStacks();
        repaint();
        varOwner->generateVarsTree();
        emit openProject();

        QThread::msleep(500);
        on_closeInfoListButton_clicked();
        return 1;
    }
    addMessageToInfoList(QDateTime::currentDateTime().time().toString() + " :error - Ошибка открытия файла");
    return 0;
}

void MainWindow::saveFileByName(const QString &fName)
{
    QFile file(fName);
    if (file.open(QIODevice::WriteOnly)) {

        QFileInfo fInfo(fName);
        prDirPath = fInfo.absolutePath();
        prFileName = fInfo.fileName();

        QStringList prevPr = getPrevProjects();
        prevPr.insert(0,fName);
        updatePrevProjects(prevPr);

        QTextStream out(&file);
        out.setCodec("Windows-1251");
        for(int i=0;i<editor->blockCount();i++) {
            out << editor->document()->findBlockByNumber(i).text();
            out << "\r\n";
        }
        file.close();
        setWindowTitle(wTitle + " - " + fName);
        RCompiler::setInpDirName(fInfo.dir().path());
        RCompiler::setInpKonFileName(fInfo.fileName());
        prChangedFlag = false;
        if(settings!=nullptr) {
            settings->setKonFileName(fName);
            settings->saveSettings();
        }
        emit saveProject();
    }
}

void MainWindow::activateInfoPanel()
{
    ui->listWidget->clear();
    ui->listWidget->setVisible(true);
    ui->closeInfoListButton->setVisible(true);
    ui->horizontalSpacer->changeSize(1,1, QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->infoLabel->setVisible(true);
}

int MainWindow::saveWarning()
{
    if(prChangedFlag) {
        QMessageBox msgBox(this);
        msgBox.setText("Проект был изменён");
        msgBox.setInformativeText("Вы хотите сохранить изменения?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        int ret = msgBox.exec();
        switch (ret) {
          case QMessageBox::Save:
              saveFile();
              break;
          case QMessageBox::Discard:
              // Don't Save was clicked
              break;
          case QMessageBox::Cancel:
              return 0;
          default:
              // should never be reached
              break;
        }
    }
    return 1;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->listWidget->setVisible(false);
    ui->closeInfoListButton->setVisible(false);
    ui->infoLabel->setVisible(false);
    ui->horizontalSpacer->changeSize(0,0, QSizePolicy::Fixed, QSizePolicy::Fixed);

    setWindowTitle(wTitle + " - несохранённый проект");
    prDirPath="";
    prFileName="";

    editor = new CodeEditor(this);
    Highlighter* highlighter = new Highlighter(editor->document());
    Q_UNUSED(highlighter);

    newAct = new QAction(QIcon("://new_32.ico"), "Новый проект", this);
    openAct = new QAction(QIcon("://open_32.ico"), "Открыть", this);
    saveAct = new QAction(QIcon("://save_32.ico"), "Сохранить", this);
    saveAsAct = new QAction(QIcon("://save_32.ico"), "Сохранить как", this);
    undoAct = new QAction(QIcon("://undo_32.ico"), "Отменить операцию", this);
    redoAct = new QAction(QIcon("://redo_32.ico"), "Повторить отменённую операцию", this);
    srchAct = new QAction(QIcon("://srch_32.ico"), "Искать текст", this);
    buildAct = new QAction(QIcon("://build_32.ico"), "Собрать проект", this);
    toPlcAct = new QAction(QIcon("://toPLC_32.ico"), "Загрузить проект в контроллер", this);

    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));
    connect(openAct, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(saveFile()));
    connect(undoAct, SIGNAL(triggered()), this, SLOT(undo()));
    connect(redoAct, SIGNAL(triggered()), this, SLOT(redo()));
    connect(srchAct, SIGNAL(triggered()), this, SLOT(searchText()));
    connect(buildAct, SIGNAL(triggered()), this, SLOT(buildPr()));
    connect(toPlcAct, SIGNAL(triggered()), this, SLOT(projectToPlc()));
    connect(saveAsAct,SIGNAL(triggered()), this, SLOT(saveAsFile()));

    ui->mainToolBar->addAction(newAct);
    ui->mainToolBar->addAction(openAct);
    ui->mainToolBar->addAction(saveAct);
    ui->mainToolBar->addAction(undoAct);
    ui->mainToolBar->addAction(redoAct);
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->mainToolBar->addAction(srchAct);


    textForSearch = new QLineEdit("");
    textForSearch->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    ui->mainToolBar->addWidget(textForSearch);
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(buildAct);
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(toPlcAct);
    ui->mainToolBar->addWidget(spacer);

    QStringList prNames = getPrevProjects();
    if(prNames.count()) {
        updatePrevProjects(prNames);

    }

    connect(textForSearch,SIGNAL(returnPressed()),this,SLOT(searchText()));

    QFont font("Courier",10,QFont::Normal,false);
    const int tabWidth = 4;  // 4 characters
    QFontMetrics metrics(font);
    editor->setTabStopWidth(tabWidth * metrics.width(' '));
    editor->setFont(font);

    ui->tabWidget->clear();
    ui->tabWidget->tabBar()->setFont(QFont("Courier",12,QFont::Normal,false));
    ui->tabWidget->addTab(editor,"Редактор");
    editor->setFocus();

    varOwner = new VarsCreator();

    settings = new SettingsForm();
    ui->tabWidget->addTab(settings,"Настройки");

    debugger = new DebuggerForm(*varOwner);
    connect(this,SIGNAL(openProject()),debugger,SLOT(openProject()));
    connect(this,SIGNAL(newProject()),debugger,SLOT(newProject()));
    connect(this,SIGNAL(saveProject()),debugger,SLOT(saveProject()));
    ui->tabWidget->addTab(debugger,"Отладчик");

    displ = new Display();
    lcd = new LCDForm(*displ,*varOwner);
    connect(this,SIGNAL(newProject()),lcd,SIGNAL(newProject()));
    connect(this,SIGNAL(openProject()),lcd,SIGNAL(openProject()));
    connect(this,SIGNAL(saveProject()),lcd,SIGNAL(saveProject()));
    ui->tabWidget->addTab(lcd,"Пульт");

    /*ui->mdiArea->addSubWindow(editor);
    ui->mdiArea->addSubWindow(debugger);
    ui->mdiArea->addSubWindow(settings);
    ui->mdiArea->tileSubWindows();//setViewMode(QMdiArea::TabbedView);
    */


    setWindowState(Qt::WindowMaximized);

    PrBuilder *buildProc = new PrBuilder;
    buildProc->moveToThread(&builderThread);
    connect(&builderThread, SIGNAL(finished()), buildProc, SLOT(deleteLater()));
    connect(this, SIGNAL(startBuild(QString,QString)), buildProc, SLOT(buildRequest(QString,QString)));
    connect(this,SIGNAL(updateKonFileForBuilder(QStringList)), buildProc, SLOT(setFileText(QStringList)));
    connect(buildProc, SIGNAL(printMessage(QString)), this, SLOT(addMessageToInfoList(QString)));
    connect(buildProc, SIGNAL(buildIsOk()),this,SLOT(buildWithoutErrors()));
    builderThread.start();

    newFile();
    QThread::msleep(1000);
    prChangedFlag = false;
    connect(editor,SIGNAL(textChanged()),this,SLOT(prWasChanged()));
}

MainWindow::~MainWindow()
{
    builderThread.quit();
    builderThread.wait();
    delete ui;
    delete displ;
    delete varOwner;
}

void MainWindow::newFile()
{
    if(saveWarning()==0) return;

    addMessageToInfoList(QDateTime::currentDateTime().time().toString() + " :Новый проект");

    editor->clear();
    editor->appendPlainText("#DATA //-----Присвоение переменных программы.");
    editor->appendPlainText("    ");
    editor->appendPlainText("#INIT //-----Инициализация данных.");
    editor->appendPlainText("    ");
    editor->appendPlainText("#STARTp0;");
    editor->appendPlainText("    ");
    editor->appendPlainText("//-----Начало программы.");
    editor->appendPlainText("#PROCESS 0");
    editor->appendPlainText("#SIT1(0.1)");
    editor->appendPlainText("    ");
    editor->appendPlainText("#/R;");

    // - путь по умолчанию
    QDir dir(QApplication::applicationDirPath()+"/newProject");
    if(!dir.exists()) {
        dir.mkdir(".");
    }
    saveFileByName(dir.absolutePath()+"/project.kon");
    //

    prChangedFlag = false;
    if(settings!=nullptr) {
        //settings->setKonFileName("");
        settings->clearSettings();
    }
    editor->document()->clearUndoRedoStacks();
    varOwner->generateVarsTree();
    emit newProject();

    //RCompiler::setInpDirName("");
    //RCompiler::setInpKonFileName("");


}

void MainWindow::openFile()
{
    if(saveWarning()==0) return;

    QStringList prevProjects = getPrevProjects();
    QString path = "/";
    if(prevProjects.count()) {
        if(QFile::exists(prevProjects.at(0))) {
            QFileInfo fInfo(prevProjects.at(0));
            path = fInfo.absolutePath();
        }
    }

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    path,
                                                    tr("Relkon Files (*.kon )"));
    if(fileName.isEmpty()) return;
    openFileByName(fileName);
    //QThread::msleep(1000);
    prChangedFlag = false;
    //editor->document()->clearUndoRedoStacks();
}

void MainWindow::saveFile()
{
    QStringList prevProjects = getPrevProjects();
    QString path = "/";
    if(prevProjects.count()) {
        if(QFile::exists(prevProjects.at(0))) {
            QFileInfo fInfo(prevProjects.at(0));
            path = fInfo.absolutePath();
        }
    }
    QString fileName;
    if(prFileName.isEmpty())
    fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    path,
                                                    tr("Relkon Files (*.kon )"));
    else {
        fileName = RCompiler::getKonFileName();

        QFile file(fileName);
        QFileInfo fInfo(file);
        QDir dir(fInfo.dir().path()+ "/back");
        if(!dir.exists()) {
            dir.mkdir(".");
        }
        QString backName = QDateTime::currentDateTime().toString();
        backName.replace(QRegExp("[\\s\\t\\.:]"),"_");
        backName=fInfo.dir().path() + "/back/" +backName+".kon";
        QFile::copy(fileName,backName);
    }
    saveFileByName(fileName);
}

void MainWindow::saveAsFile()
{
    QStringList prevProjects = getPrevProjects();
    QString path = "/";
    if(prevProjects.count()) {
        if(QFile::exists(prevProjects.at(0))) {
            QFileInfo fInfo(prevProjects.at(0));
            path = fInfo.absolutePath();
        }
    }
    QString fileName;
    fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    path,
                                                    tr("Relkon Files (*.kon )"));
    saveFileByName(fileName);
}

void MainWindow::undo()
{
    editor->undo();
}

void MainWindow::redo()
{
    editor->redo();
}

void MainWindow::searchText()
{
    QTextCursor highlightCursor = editor->textCursor();

    highlightCursor = editor->document()->find(textForSearch->text(), highlightCursor, QTextDocument::FindWholeWords);

    if (!highlightCursor.isNull()) {
        QTextCursor startPosCursor = highlightCursor;
        highlightCursor.movePosition(QTextCursor::WordRight,
                            QTextCursor::KeepAnchor);

        QTextBlock block = startPosCursor.block();
        if(!block.isVisible()) {
            while(!block.isVisible()) {
                block = block.previous();
                if(block.isValid()) {
                    if(block.isVisible()) {
                        editor->toggleFolding(block);
                        break;
                    }
                }
            }
        }
        editor->setTextCursor(startPosCursor);
    }
}

void MainWindow::buildPr()
{
    saveFile();
    activateInfoPanel();

    if(ui->listWidget->isVisible()) {
        QStringList conFile;
        for(int i=0;i<editor->blockCount();i++) {
            conFile << editor->document()->findBlockByNumber(i).text() + " ";
        }
        emit updateKonFileForBuilder(conFile);
        emit startBuild(prDirPath,prFileName);
    }
}

void MainWindow::projectToPlc()
{
    if(QFile::exists(RCompiler::getBinFileName())) {
        ScanGUI gui(settings->getProgAddr(),this);
        int ret = gui.exec();
        if(ret==QDialog::Accepted) {
            BootModeSetter bootSetter(this);
            if(bootSetter.setBootMode()) {
                DetectedController* plc = &DetectedController::Instance();
                YmodemThread loader(plc->getUartName(),this);
                loader.exec();
            }
        }
    }else QMessageBox::warning(this,"Загрузка","Ошибка открытия файла "+RCompiler::getBinFileName());
}

void MainWindow::addMessageToInfoList(const QString &message)
{
    ui->listWidget->addItem(message);
    if(message.contains("error")) {
        ui->listWidget->item(ui->listWidget->count()-1)->setTextColor(Qt::darkRed);
        ui->listWidget->item(ui->listWidget->count()-1)->setFont (QFont ("Courier", 10,QFont::DemiBold));
    }else if(message.contains("warning")||message.contains("note")) {
        ui->listWidget->item(ui->listWidget->count()-1)->setTextColor(QColor(139,69,19));
        ui->listWidget->item(ui->listWidget->count()-1)->setFont (QFont ("Courier", 10,QFont::Normal));
    }else {
        ui->listWidget->item(ui->listWidget->count()-1)->setTextColor(QColor(0,50,0));
        ui->listWidget->item(ui->listWidget->count()-1)->setFont (QFont ("Courier", 10,QFont::Normal));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(saveWarning()==0) event->ignore();
    else event->accept();
}

void MainWindow::on_closeInfoListButton_clicked()
{
    ui->listWidget->setVisible(false);
    ui->closeInfoListButton->setVisible(false);
    ui->infoLabel->setVisible(false);
    ui->horizontalSpacer->changeSize(0,0, QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void MainWindow::on_listWidget_doubleClicked(const QModelIndex &index)
{
    QString str = index.data().toString();
    QRegExp pattern("строка:(\\d+)");
    if(pattern.indexIn(str)!=-1) {
        int strNum = pattern.cap(1).toInt();
        if((strNum>0)&&(strNum<=editor->blockCount())) {
            QTextBlock block = editor->document()->findBlockByNumber(strNum-1);
            QTextCursor cursor(block);
            if(!block.isVisible()) {
                while(!block.isVisible()) {
                    block = block.previous();
                    if(block.isValid()) {
                        if(block.isVisible()) {
                            editor->toggleFolding(block);
                            break;
                        }
                    }
                }
            }
            editor->setTextCursor(cursor);
        }
    }
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if(index!=0) {
        undoAct->setEnabled(false);
        redoAct->setEnabled(false);
        srchAct->setEnabled(false);
        buildAct->setEnabled(false);
        textForSearch->setEnabled(false);
        if(index==3) lcd->updFocus();
    }else {
        undoAct->setEnabled(true);
        redoAct->setEnabled(true);
        srchAct->setEnabled(true);
        buildAct->setEnabled(true);
        textForSearch->setEnabled(true);
    }
}

void MainWindow::openPrevProject()
{
    QAction *a = qobject_cast<QAction *>(sender());
    if(a!=nullptr) {
        QString fName = a->text();
        if(saveWarning()==0) return;
        openFileByName(fName);
        //QThread::msleep(1000);
        prChangedFlag = false;
    }
}

void MainWindow::prWasChanged()
{
    prChangedFlag = true;
}

void MainWindow::buildWithoutErrors()
{
    QThread::msleep(1000);
    on_closeInfoListButton_clicked();
}
