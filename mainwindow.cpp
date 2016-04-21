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
#include <QFontDialog>
#include "dialogeditguisettings.h"
#include "Help/aboutdialog.h"
#include <QProcess>
#include "Search/searchdialog.h"
#include "pathstorage.h"
#include "settingsbase.h"

#include "rp6creator.h"




QStringList MainWindow::getPrevProjects()
{
    QStringList res;
    QSettings settings;//("Kontel","RIDE");
    res = settings.value("/Settings/PrevProjects",QStringList()).toStringList();
    res.removeDuplicates();
    if(res.count()>maxAmountOfPrevProjects) res = res.mid(0,maxAmountOfPrevProjects);
    settings.setValue("/Settings/PrevProjects",res);
    return res;
}

void MainWindow::updatePrevProjects(const QStringList &prNames)
{
    QSettings settings;//("Kontel","RIDE");
    settings.setValue("/Settings/PrevProjects",prNames);
    QStringList resList = getPrevProjects();
    ui->menuFile->clear();

    ui->menuFile->addAction(newAct);
    ui->menuFile->addAction(openAct);
    ui->menuFile->addAction(saveAct);
    ui->menuFile->addAction(saveAsAct);
    ui->menuFile->addAction(closeProjectAct);
    QMenu *impMenu = ui->menuFile->addMenu("Import");
    impMenu->addAction(importPultAct);
    impMenu->addAction(openSysFramFromRelkon6);
    impMenu->addAction(importPrAction);
    QMenu *expMenu = ui->menuFile->addMenu("Export");
    expMenu->addAction(rp6Act);
    ui->menuFile->addMenu(expMenu);
    QMenu* recPr = new QMenu("Недавние проекты");
    ui->menuFile->addMenu(recPr);
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
        editor->foldAll();

        QTextCursor tmpCursor = editor->textCursor();
        tmpCursor.setVisualNavigation(true);
        tmpCursor.setPosition(editor->document()->firstBlock().position());
        editor->setTextCursor(tmpCursor);
        file.close();
        addMessageToInfoList(QDateTime::currentDateTime().time().toString() + " :Файл успешно открыт");
        setWindowTitle("(" + prFileName+") " + wTitle + " - " + fName);

        PathStorage::setPrDir(fInfo.dir().path());
        PathStorage::setKonFileName(fInfo.fileName());

        if(settings!=nullptr) {
            settings->clearSettings();
            settings->setKonFileName(fName);
            settings->openSettings();
            debugger->setNetAddress(settings->getNetAddr());
        }
        editor->document()->clearUndoRedoStacks();

        repaint();
        varOwner->generateVarsTree();
        emit openProject();

        //QThread::msleep(500);
        on_closeInfoListButton_clicked();

        prChangedFlag = false;
        ui->tabWidget->setTabText(0,"Редактор");
        ui->tabWidget->setEnabled(true);
        noProject = false;
        connect(editor,SIGNAL(textChanged()),this,SLOT(prWasChanged()));
        enableActionWithProject();
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
        //setWindowTitle(wTitle + " - " + fName);
        setWindowTitle("(" + prFileName+") " + wTitle + " - " + fName);

        PathStorage::setPrDir(fInfo.dir().path());
        PathStorage::setKonFileName(fInfo.fileName());

        prChangedFlag = false;
        ui->tabWidget->setTabText(0,"Редактор");
        if(settings!=nullptr) {
            settings->setKonFileName(fName);
            settings->saveSettings();
        }
        emit saveProject();
        addMessageToInfoList(QDateTime::currentDateTime().time().toString() + ": проект успешно сохранён");
    }
}

void MainWindow::activateInfoPanel()
{

    //ui->listWidget->clear();
    ui->listWidget->setVisible(true);
    ui->closeInfoListButton->setVisible(true);
    ui->horizontalSpacer->changeSize(1,1, QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->infoLabel->setVisible(true);
}

void MainWindow::toggleInfoPanel()
{
    if(ui->listWidget->isVisible()) on_closeInfoListButton_clicked();
    else {
        //ui->listWidget->clear();
        ui->listWidget->setVisible(true);
        ui->closeInfoListButton->setVisible(true);
        ui->horizontalSpacer->changeSize(1,1, QSizePolicy::Expanding, QSizePolicy::Fixed);
        ui->infoLabel->setVisible(true);
    }
}

void MainWindow::wrSysFramSlot()
{
    debugger->stopDebugger();
    settings->writeSysFram();
}

void MainWindow::rdSysFramSlot()
{
    debugger->stopDebugger();
    settings->readSysFram();
}

void MainWindow::emuModeChanged(SettingsBase::emuType value)
{
    if(value==SettingsBase::NoEmulation) noEmuAct->setIcon(QIcon(":/no_emu_on.ico"));else noEmuAct->setIcon(QIcon(":/no_emu_off.ico"));
    if(value==SettingsBase::InputEmulation) emuInpAct->setIcon(QIcon(":/inp_emu_on.ico"));else emuInpAct->setIcon(QIcon(":/inp_emu_off.ico"));
    if(value==SettingsBase::InputOutputEmulation) emuAct->setIcon(QIcon(":/all_emu_on.ico"));else emuAct->setIcon(QIcon(":/all_emu_off.ico"));
}

void MainWindow::emuInpActSlot()
{
    if(settings!=nullptr) settings->setEmuMode(SettingsBase::InputEmulation);
}

void MainWindow::noEmuActSlot()
{
    if(settings!=nullptr) settings->setEmuMode(SettingsBase::NoEmulation);
}

void MainWindow::emuInpOutpActSlot()
{
    if(settings!=nullptr) settings->setEmuMode(SettingsBase::InputOutputEmulation);
}

void MainWindow::createRPFile()
{
    if(RP6Creator::createRelkon6ProjectFile(varOwner)) addMessageToInfoList("файл project.rp6 успешно создан");
    else addMessageToInfoList("ошибка создания файла project.rp6");
    activateInfoPanel();
}

void MainWindow::toggleDebugger()
{
    debugger->tabChanged();
    dockDebugger->raise();
}

void MainWindow::togglePult()
{
    lcd->updFocus();
    dockDisplay->raise();
}

void MainWindow::toggleSettings()
{
    dockSettings->raise();
}

int MainWindow::saveWarning()
{
    if(prChangedFlag || displ->getChanged()) {
        QMessageBox msgBox(this);
        msgBox.setText("Проект был изменён");
        msgBox.setInformativeText("Вы хотите сохранить изменения?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        msgBox.setButtonText(QMessageBox::Save, "сохранить");
        msgBox.setButtonText(QMessageBox::Discard, "не сохранять");
        msgBox.setButtonText(QMessageBox::Cancel, "отменить");
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

QString MainWindow::getFileNameForPultImport()
{
    QStringList prevProjects = getPrevProjects();
    QString path = "/";
    if(prevProjects.count()) {
        if(QFile::exists(prevProjects.at(0))) {
            QFileInfo fInfo(prevProjects.at(0));
            path = fInfo.absolutePath();
        }
    }
    QString fName = QFileDialog::getOpenFileName(this, tr("Импорт пульта"),
                                                    path,
                                                    tr("Relkon 6.x Pult (*.plt )"));
    return fName;
}

void MainWindow::insertVar(QDomElement &e, int strNum)
{
    if(!e.isNull()) {
        QString varName = e.attribute("Name");
        QString varSign = e.attribute("HasSign");
        QString varReadOnly = e.attribute("ReadOnly");
        QString varPos = e.attribute("Position");
        QString varPattern = e.attribute("Mask");
        if(!varName.isEmpty()) {
            PultVarDefinition vDef;
            varPattern.replace(',','.');
            vDef.setEditable(varReadOnly=="true"?false:true);
            vDef.setPattern(varPattern);
            vDef.setPosInStr(varPos.toInt());
            vDef.setForceSign(varSign=="true"?true:false);
            displ->setCursor(varPos.toInt(),strNum);

            if(varName=="DATE") varName="sysTime_date";
            if(varName=="MONTH") varName="sysTime_month";
            if(varName=="YEAR") varName="sysTime_year";
            if(varName=="HOUR") varName="sysTime_hour";
            if(varName=="MIN") varName="sysTime_min";
            if(varName=="SEC") varName="sysTime_sec";

            QRegExp eeExp("^EE(\\d+)");
            if(eeExp.indexIn(varName) != -1) {
                int num = eeExp.cap(1).toInt();
                vDef.setEEVar(true);
                vDef.setEEposInSettingsTable(num);
            }else {
                vDef.setEEVar(false);
            }

            vDef.setName(varName);

            displ->addVar(vDef, false);
        }
    }
}

void MainWindow::insertTextData(const QString &txt, int strNum, const LCDPhont &ph)
{
    displ->setCursor(0,strNum);
    for(int k=0;k<displ->getLength();k++) {

        if(k<txt.length()) {
            int unicodeValue = txt.at(k).unicode();
            displ->insertSymbol(ph.getSymbCodeinPhont(unicodeValue));
        }
    }
}

void MainWindow::insertStr(QDomNodeList &views, int strNum, int subStrNum, const LCDPhont &ph)
{
    displ->addEmptyStrAfter(strNum,displ->getSubStrCount(strNum)-1, false);
    QDomNode vn = views.item(subStrNum);
    QDomElement ve = vn.toElement();
    if(!ve.isNull()) {
        QString strText = ve.attribute("Text");
        QString enFlag = ve.attribute("Enabled");
        if((!strText.isEmpty())&&(!enFlag.isEmpty())) {
            insertTextData(strText,strNum,ph);
        }
        if(enFlag=="false") displ->toggleActive(strNum,subStrNum);
        QDomNodeList varsParent = vn.childNodes();
        if(varsParent.count()==1) {
            QDomNode vp = varsParent.item(0);
            QDomNodeList vars = vp.childNodes();

            if(vars.count()) {
                displ->setReplaceMode(true);
                for(int y=0;y<vars.count();y++) {
                    QDomNode varNode = vars.item(y);
                    QDomElement varElement = varNode.toElement();
                    insertVar(varElement,strNum);
                }
                displ->setReplaceMode(false);
            }
        }
    }
}

void MainWindow::createToolbar()
{
    newAct = new QAction(QIcon("://new_32.ico"), "Новый проект", this);
    openAct = new QAction(QIcon("://open_32.ico"), "Открыть", this);
    importPultAct = new QAction(QIcon("://import_pult.ico"), "Загрузить пульт версии 6.x",this);
    progrAllAct = new QAction(QIcon("://allToPLC.ico"), "Загрузить программу и настройки в контроллер F6",this);
    saveAct = new QAction(QIcon("://save_32.ico"), "Сохранить", this);
    saveAsAct = new QAction(QIcon("://save_32.ico"), "Сохранить как", this);
    undoAct = new QAction(QIcon("://undo_32.ico"), "Отменить операцию", this);
    redoAct = new QAction(QIcon("://redo_32.ico"), "Повторить отменённую операцию", this);
    srchAct = new QAction(QIcon("://srch_32.ico"), "Искать текст", this);
    buildAct = new QAction(QIcon("://build_32.ico"), "Собрать проект F5", this);
    toPlcAct = new QAction(QIcon("://toPLC_32.ico"), "Загрузить программу в контроллер F7", this);
    editGUI = new QAction(QIcon("://config.ico"), "Настройки среды", this);
    closeProjectAct = new QAction(QIcon("://close.ico"), "Закрыть проект", this);

    noEmuAct = new QAction(QIcon("://no_emu_off.ico"), "без эмуляции", this);
    emuAct = new QAction(QIcon("://all_emu_off.ico"), "полная эмуляция", this);
    emuInpAct = new QAction(QIcon("://inp_emu_off.ico"), "эмуляция входов", this);

    sysMessAction = new QAction(QIcon("://info.ico"),"Системные сообщения", this);
    connect(sysMessAction,SIGNAL(triggered(bool)),this,SLOT(toggleInfoPanel()));

    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));
    connect(openAct, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(saveFile()));
    connect(undoAct, SIGNAL(triggered()), this, SLOT(undo()));
    connect(redoAct, SIGNAL(triggered()), this, SLOT(redo()));
    connect(srchAct, SIGNAL(triggered()), this, SLOT(searchText()));
    connect(buildAct, SIGNAL(triggered()), this, SLOT(buildPr()));
    connect(toPlcAct, SIGNAL(triggered()), this, SLOT(projectToPlc()));
    connect(saveAsAct,SIGNAL(triggered()), this, SLOT(saveAsFile()));
    connect(importPultAct, SIGNAL(triggered()), this, SLOT(importPult()));
    connect(closeProjectAct, SIGNAL(triggered()), this, SLOT(closeProject()));
    connect(editGUI, SIGNAL(triggered()),this, SLOT(editIDESettings()));
    connect(progrAllAct, SIGNAL(triggered(bool)), this, SLOT(progrAll()));

    connect(noEmuAct, SIGNAL(triggered(bool)), this, SLOT(noEmuActSlot()));
    connect(emuAct, SIGNAL(triggered(bool)), this, SLOT(emuInpOutpActSlot()));
    connect(emuInpAct, SIGNAL(triggered(bool)), this, SLOT(emuInpActSlot()));

    ui->mainToolBar->addAction(newAct);
    ui->mainToolBar->addAction(openAct);
    ui->mainToolBar->addAction(saveAct);
    //ui->mainToolBar->addAction(closeProjectAct);
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(undoAct);
    ui->mainToolBar->addAction(redoAct);
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);



    ui->mainToolBar->addAction(srchAct);

    toTableAction = new QAction(QIcon("://table.ico"), "пульт > настройки", this);
    //QAction *fromTableAction = new QAction(QIcon("://display.ico"), "настройки > пульт", this);

    //ui->menuCmd->addAction(fromTableAction);
    connect(toTableAction,SIGNAL(triggered()),this,SLOT(lcdToTable()));
    //connect(fromTableAction,SIGNAL(triggered()),this,SLOT(tableToLcd()));
    ui->menuCmd->addAction(buildAct);
    ui->menuCmd->addAction(progrAllAct);
    ui->menuCmd->addAction(toPlcAct);
    wrSettings = new QAction(QIcon("://writeData.png"),"Записать настройки F8",this);
    rdSettings = new QAction(QIcon("://readData.png"),"Прочитать настройки F9",this);
    connect(wrSettings,SIGNAL(triggered(bool)),this,SLOT(wrSysFramSlot()));
    connect(rdSettings,SIGNAL(triggered(bool)),this,SLOT(rdSysFramSlot()));
    ui->menuCmd->addAction(wrSettings);
    ui->menuCmd->addAction(rdSettings);
    ui->menuCmd->addAction(toTableAction);

    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(buildAct);
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(progrAllAct);
    ui->mainToolBar->addAction(toPlcAct);
    ui->mainToolBar->addAction(wrSettings);
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(foldAction);
    ui->mainToolBar->addAction(unfoldAction);
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(noEmuAct);
    ui->mainToolBar->addAction(emuInpAct);
    ui->mainToolBar->addAction(emuAct);
    ui->mainToolBar->addAction(sysMessAction);
    ui->mainToolBar->addSeparator();
}

void MainWindow::setEditorPhont()
{
    QSettings sysSettings;
    QString edFontName = sysSettings.value("/Settings/edFontName","Courier").toString();
    int edFontSize = sysSettings.value("Settings/edFontSize",10).toInt();

    QFont font(edFontName,edFontSize,QFont::Normal,false);
    const int tabWidth = 4;  // 4 characters
    QFontMetrics metrics(font);
    editor->setTabStopWidth(tabWidth * metrics.width(' '));
    editor->setFont(font);
}

void MainWindow::createDebugger()
{
    debugger = new DebuggerForm(*varOwner);
    connect(this,SIGNAL(openProject()),debugger,SLOT(openProject()));
    connect(this,SIGNAL(newProject()),debugger,SLOT(newProject()));
    connect(this,SIGNAL(saveProject()),debugger,SLOT(saveProject()));
    connect(this,SIGNAL(updTree()),debugger,SLOT(updTree()));
    //ui->tabWidget->addTab(debugger,"Отладчик");

    dockDebugger = new QDockWidget(tr("Отладчик F4"), this);
    dockDebugger->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    dockDebugger->setWidget(debugger);
    addDockWidget(Qt::RightDockWidgetArea, dockDebugger);
    //ui->mainToolBar->addAction(dockDebugger->toggleViewAction());
    ui->menuView->addAction(dockDebugger->toggleViewAction());
    ui->mainToolBar->addAction(dockDebugger->toggleViewAction());
    connect(dockDebugger->toggleViewAction(),SIGNAL(triggered(bool)),this,SLOT(toggleDebugger()));
    dockDebugger->close();
}

void MainWindow::createDisplay()
{
    displ = new Display();
    lcd = new LCDForm(*displ,*varOwner);
    connect(this,SIGNAL(newProject()),lcd,SIGNAL(newProject()));
    connect(this,SIGNAL(openProject()),lcd,SIGNAL(openProject()));
    connect(this,SIGNAL(saveProject()),lcd,SIGNAL(saveProject()));
    connect(this,SIGNAL(updTree()),lcd,SIGNAL(updTree()));
    //ui->tabWidget->addTab(lcd,"Пульт");
    dockDisplay = new QDockWidget(tr("Пульт F3"), this);
    dockDisplay->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    dockDisplay->setWidget(lcd);
    addDockWidget(Qt::RightDockWidgetArea, dockDisplay);
    ui->menuView->addAction(dockDisplay->toggleViewAction());
    ui->mainToolBar->addAction(dockDisplay->toggleViewAction());
    connect(dockDisplay->toggleViewAction(),SIGNAL(triggered(bool)),this,SLOT(togglePult()));
    dockDisplay->close();
}

void MainWindow::createBuilder()
{
    PrBuilder *buildProc = new PrBuilder(*displ);
    buildProc->moveToThread(&builderThread);
    connect(&builderThread, SIGNAL(finished()), buildProc, SLOT(deleteLater()));
    connect(this, SIGNAL(startBuild(QString,QString)), buildProc, SLOT(buildRequest(QString,QString)));
    connect(this,SIGNAL(updateKonFileForBuilder(QStringList)), buildProc, SLOT(setFileText(QStringList)));
    connect(buildProc, SIGNAL(printMessage(QString)), this, SLOT(addMessageToInfoList(QString)));
    connect(buildProc, SIGNAL(buildIsOk()),this,SLOT(buildWithoutErrors()));
    builderThread.start();
}

void MainWindow::createHelp()
{
    helpBrAct = new QAction(QIcon("://help.ico"), "Меню", this);
    ui->menuHelp->addAction(helpBrAct);
    QAction *aboutAction = new QAction(QIcon("://about.ico"), "О программе", this);
    connect(aboutAction,SIGNAL(triggered()),this,SLOT(viewAboutWindow()));
    ui->menuHelp->addAction(aboutAction);

    helpBr = new HelpBrowser(QApplication::applicationDirPath()+"/Doc","index.html");
    connect(helpBrAct,SIGNAL(triggered()),this,SLOT(viewHelp()));
}

void MainWindow::createUtilities()
{
    QAction *actR = new QAction(QIcon("://reloader.ico"), "Загрузчик", this);
    ui->menuUtil->addAction(actR);
    connect(actR,SIGNAL(triggered()),this,SLOT(startReloader()));
    QAction *actMMB = new QAction(QIcon("://mmb.ico"), "Настройка Matchbox", this);
    ui->menuUtil->addAction(actMMB);
    connect(actMMB,SIGNAL(triggered()),this,SLOT(startMMBConfig()));
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

    setWindowTitle(wTitle);
    prDirPath="";
    prFileName="";

    editor = new CodeEditor(this);
    Highlighter* highlighter = new Highlighter(editor->document());
    Q_UNUSED(highlighter);
    setEditorPhont();

    foldAction = new QAction(QIcon("://contract.ico"),"свернуть всё",this);
    unfoldAction = new QAction(QIcon("://expand.ico"),"развернуть всё",this);
    connect(foldAction,SIGNAL(triggered(bool)),editor,SLOT(foldAll()));
    connect(unfoldAction,SIGNAL(triggered(bool)),editor,SLOT(unfoldAll()));

    rp6Act = new QAction("rp6 для панели", this);
    connect(rp6Act,SIGNAL(triggered(bool)),this,SLOT(createRPFile()));

    createToolbar();

    openSysFramFromRelkon6 = new QAction(QIcon("://database.ico"), "Загрузить зав. установки из Relkon 6.x", this);
    connect(openSysFramFromRelkon6,SIGNAL(triggered(bool)),this,SLOT(loadSysFramRelk6()));
    importPrAction = new QAction(QIcon("://import.ico"), "Открыть проект версии 6.x", this);
    connect(importPrAction,SIGNAL(triggered(bool)),this,SLOT(importProject()));



    ui->menuEdit->addAction(srchAct);
    ui->menuEdit->addAction(foldAction);
    ui->menuEdit->addAction(unfoldAction);
    ui->menuEdit->addAction(editGUI);

    QStringList prNames = getPrevProjects();
    if(prNames.count()) {
        updatePrevProjects(prNames);
    }

    PathStorage::setCoreDir(QApplication::applicationDirPath() + "/core");

    ui->tabWidget->clear();
    ui->tabWidget->tabBar()->setFont(QFont("Courier",12,QFont::Normal,false));
    ui->tabWidget->addTab(editor,"Редактор");
    editor->setFocus();


    varOwner = new VarsCreator();
    settings = new SettingsForm();
    connect(settings,SIGNAL(emuModeChanged(SettingsBase::emuType)),this,SLOT(emuModeChanged(SettingsBase::emuType)));
    PathStorage::setBuildName(settings->getBuildName());

    dockSettings = new QDockWidget(tr("Настройки F2"), this);
    dockSettings->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    dockSettings->setWidget(settings);
    addDockWidget(Qt::RightDockWidgetArea, dockSettings);
    ui->menuView->addAction(dockSettings->toggleViewAction());
    ui->mainToolBar->addAction(dockSettings->toggleViewAction());
    dockSettings->close();
    connect(dockSettings->toggleViewAction(),SIGNAL(triggered(bool)),this,SLOT(toggleSettings()));


    //ui->tabWidget->addTab(settings,"Настройки");

    createDisplay();
    createDebugger();


    ui->menuView->addAction(sysMessAction);
    //ui->mainToolBar->addWidget(spacer);

    QMainWindow::setDockOptions(QMainWindow::ForceTabbedDocks);
    QMainWindow::tabifyDockWidget(dockSettings, dockDisplay);
    QMainWindow::tabifyDockWidget(dockDisplay, dockDebugger);

    /*ui->mdiArea->addSubWindow(editor);
    ui->mdiArea->addSubWindow(debugger);
    ui->mdiArea->addSubWindow(settings);
    ui->mdiArea->tileSubWindows();//setViewMode(QMdiArea::TabbedView);*/


    setWindowState(Qt::WindowMaximized);
    createBuilder();
    createHelp();
    createUtilities();
    //newFile();
    ui->tabWidget->setEnabled(false);
    //QThread::msleep(1000);
    prChangedFlag = false;
    ui->tabWidget->setTabText(0,"Редактор");
    disableActionWithoutProject();
    //connect(editor,SIGNAL(textChanged()),this,SLOT(prWasChanged()));
}

MainWindow::~MainWindow()
{
    builderThread.quit();
    builderThread.wait();
    delete ui;
    delete displ;
    delete varOwner;
    delete helpBr;
    delete dockDebugger;
    delete dockDisplay;
}

void MainWindow::disableActionWithoutProject()
{
    undoAct->setEnabled(false);
    redoAct->setEnabled(false);
    srchAct->setEnabled(false);
    buildAct->setEnabled(false);
    toPlcAct->setEnabled(false);
    progrAllAct->setEnabled(false);
    saveAct->setEnabled(false);
    saveAsAct->setEnabled(false);
    closeProjectAct->setEnabled(false);
    importPultAct->setEnabled(false);
    editGUI->setEnabled(false);
    openSysFramFromRelkon6->setEnabled(false);
    foldAction->setEnabled(false);
    unfoldAction->setEnabled(false);
    toTableAction->setEnabled(false);
    wrSettings->setEnabled(false);
    rdSettings->setEnabled(false);
    noEmuAct->setEnabled(false);
    emuAct->setEnabled(false);
    emuInpAct->setEnabled(false);
    rp6Act->setEnabled(false);
    sysMessAction->setEnabled(false);
    dockDebugger->toggleViewAction()->setEnabled(false);
    dockDisplay->toggleViewAction()->setEnabled(false);
    dockSettings->toggleViewAction()->setEnabled(false);
}

void MainWindow::enableActionWithProject()
{
    undoAct->setEnabled(true);
    redoAct->setEnabled(true);
    srchAct->setEnabled(true);
    buildAct->setEnabled(true);
    toPlcAct->setEnabled(true);
    progrAllAct->setEnabled(true);
    saveAct->setEnabled(true);
    saveAsAct->setEnabled(true);
    closeProjectAct->setEnabled(true);
    importPultAct->setEnabled(true);
    editGUI->setEnabled(true);
    openSysFramFromRelkon6->setEnabled(true);
    foldAction->setEnabled(true);
    unfoldAction->setEnabled(true);
    toTableAction->setEnabled(true);
    wrSettings->setEnabled(true);
    rdSettings->setEnabled(true);
    noEmuAct->setEnabled(true);
    emuAct->setEnabled(true);
    emuInpAct->setEnabled(true);
    rp6Act->setEnabled(true);
    sysMessAction->setEnabled(true);
    dockDebugger->toggleViewAction()->setEnabled(true);
    dockDisplay->toggleViewAction()->setEnabled(true);
    dockSettings->toggleViewAction()->setEnabled(true);
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
    dir.removeRecursively();

    if(!dir.exists()) {
        dir.mkdir(".");
    }
    PathStorage::setBuildName("master");

    prDirPath = dir.absolutePath();
    prFileName = "project.kon";



    saveFileByName(dir.absolutePath()+"/project.kon");
    //

    prChangedFlag = false;
    ui->tabWidget->setTabText(0,"Редактор");
    if(settings!=nullptr) {
        //settings->setKonFileName("");
        settings->clearSettings();
    }
    editor->document()->clearUndoRedoStacks();
    varOwner->generateVarsTree();
    emit newProject();

    ui->tabWidget->setEnabled(true);
    noProject = false;
    connect(editor,SIGNAL(textChanged()),this,SLOT(prWasChanged()));

    enableActionWithProject();

    buildPr();
    //RCompiler::setInpDirName("");
    //RCompiler::setInpKonFileName("");
    debugger->stopDebugger();
    debugger->closeQuickWatch();
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
    debugger->stopDebugger();
    debugger->closeQuickWatch();
    //QThread::msleep(1000);
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
        fileName = PathStorage::getKonFileFullName();
        if(prChangedFlag) {
            QDir dir(PathStorage::getBackDir());
            if(!dir.exists()) {dir.mkdir(".");}
            QString backName = QDateTime::currentDateTime().toString();
            backName.replace(QRegExp("[\\s\\t\\.:]"),"_");
            backName=PathStorage::getBackDir() + "/" + backName+".kon";
            QFile::copy(fileName,backName);
        }
        if(displ->getChanged()) {
            QString lcdFileName = fileName;
            lcdFileName.remove(".kon");
            lcdFileName += ".lcd";
            QDir dir(PathStorage::getBackDir());
            if(!dir.exists()) {dir.mkdir(".");}
            QString backName = QDateTime::currentDateTime().toString();
            backName.replace(QRegExp("[\\s\\t\\.:]"),"_");
            backName=PathStorage::getBackDir() + "/" + backName+".lcd";
            QFile::copy(lcdFileName,backName);
        }
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

void MainWindow::closeProject()
{
    if(saveWarning()==0) return;
    prChangedFlag = false;
    displ->setChanged(false);
    dockDebugger->close();
    dockDisplay->close();
    dockSettings->close();
    editor->clear();
    ui->tabWidget->setCurrentIndex(0);
    ui->tabWidget->setEnabled(false);
    noProject = true;
    disconnect(editor,SIGNAL(textChanged()),this,SLOT(prWasChanged()));
    prChangedFlag = false;
    ui->tabWidget->setTabText(0,"Редактор");
    setWindowTitle(wTitle);
    /*QAction* undoAct;
    QAction* redoAct;
    QAction* srchAct;
    QAction* buildAct;
    QAction* toPlcAct;
    QAction* newAct;
    QAction* openAct;
    QAction* saveAct;
    QAction* saveAsAct;
    QAction* closeProjectAct;
    QAction *importPultAct;
    QAction *editGUI;
    QAction *helpBrAct;
    QAction *openSysFramFromRelkon6;*/
    disableActionWithoutProject();
    debugger->stopDebugger();
    debugger->closeQuickWatch();
}

void MainWindow::undo()
{
    if(editor->hasFocus()) editor->undo();
    //else {if(dockDisplay->isEnabled()) displ->undo();}
}

void MainWindow::redo()
{
    if(editor->hasFocus()) editor->redo();
    //else {if(dockDisplay->isEnabled()) displ->redo();}
}

void MainWindow::searchText()
{
    if(SearchDialog::getCnt()==0) {
        SearchDialog *dialog = new SearchDialog(editor->getSelectedText(),this);
        connect(dialog,SIGNAL(finished(int)),dialog,SLOT(deleteLater()));
        connect(dialog,SIGNAL(startSearch(SearchData)),this,SLOT(searchCmd(SearchData)));
        connect(this,SIGNAL(searchRes(QStringList)),dialog,SLOT(getResult(QStringList)));
        connect(dialog,SIGNAL(goToStr(int,SearchData)),this,SLOT(goToStr(int,SearchData)));
        connect(dialog,SIGNAL(unfoldStr(int)),this,SLOT(unfoldStr(int)));
        connect(dialog,SIGNAL(replace(QString)),this,SLOT(replaceTxt(QString)));
        connect(dialog,SIGNAL(replaceAll(QString,QString)),this,SLOT(replaceAll(QString,QString)));
        dialog->show();
    }
}

void MainWindow::searchCmd(const SearchData &sData)
{
    QStringList sList;
    QTextDocument::FindFlags flags;
    if(sData.getCaseSensivity()) flags |= QTextDocument::FindCaseSensitively;
    if(sData.getWholeWord()) flags |= QTextDocument::FindWholeWords;
    if(sData.getSearchRegion()==SearchData::BACKWARD) flags |= QTextDocument::FindBackward;

    QTextCursor highlightCursor = editor->textCursor();
    if(sData.getWholeWord()) {
        QRegExp exp("\\b"+ sData.getSearchText() + "\\b");
        highlightCursor = editor->document()->find(exp , highlightCursor, flags);
    }else {
        highlightCursor = editor->document()->find(sData.getSearchText() , highlightCursor, flags);
    }

    int firstBlNum = -1;
    int startBlNum = -1;

    while(!highlightCursor.isNull()) {
        int blNum = highlightCursor.blockNumber();
        if(startBlNum==-1) {
            firstBlNum = startBlNum = blNum;
            editor->setTextCursor(highlightCursor);
        }
        QString strText = editor->document()->findBlockByNumber(blNum).text();
        if(sData.getWholeWord()) {
            QString exprStr = strText;
            QString searchText = sData.getSearchText();
            if(!sData.getCaseSensivity()) {
                exprStr = exprStr.toLower();
                searchText = searchText.toLower();
            }
            QRegExp exp("\\b"+ searchText + "\\b");
            if(exp.indexIn(exprStr)==-1) strText="";
        }
        if(!strText.isEmpty()) {
            strText = QString::number(blNum+1) + ": " + strText;
            sList << strText;
        }
        if(sData.getWholeWord()) {
            QRegExp exp("\\b"+ sData.getSearchText() + "\\b");
            highlightCursor = editor->document()->find(exp , highlightCursor, flags);
        }else {
            highlightCursor = editor->document()->find(sData.getSearchText() , highlightCursor, flags);
        }
    }
    if(sData.getRoundCondition()) {
        highlightCursor = editor->textCursor();
        highlightCursor.movePosition(QTextCursor::Start);
        if(flags & QTextDocument::FindBackward) {highlightCursor.movePosition(QTextCursor::End);}
        if(sData.getWholeWord()) {
            QRegExp exp("\\b"+ sData.getSearchText() + "\\b");
            highlightCursor = editor->document()->find(exp , highlightCursor, flags);
        }else {
            highlightCursor = editor->document()->find(sData.getSearchText() , highlightCursor, flags);
        }
        while(!highlightCursor.isNull()) {
            int blNum = highlightCursor.blockNumber();

            if(startBlNum != -1) {
                if(flags & QTextDocument::FindBackward) {if(blNum<startBlNum) break;}
                else if(blNum>startBlNum) break;
            }

            if(firstBlNum==-1) {
                firstBlNum=blNum;
                editor->setTextCursor(highlightCursor);
            }

            QString strText = editor->document()->findBlockByNumber(blNum).text();
            if(sData.getWholeWord()) {
                QString exprStr = strText;
                QString searchText = sData.getSearchText();
                if(!sData.getCaseSensivity()) {
                    exprStr = exprStr.toLower();
                    searchText = searchText.toLower();
                }
                QRegExp exp("\\b"+ searchText + "\\b");
                if(exp.indexIn(exprStr)==-1) strText="";
            }
            if(!strText.isEmpty()) {
                strText = QString::number(blNum+1) + ": " + strText;
                sList << strText;
            }
            if(sData.getWholeWord()) {
                QRegExp exp("\\b"+ sData.getSearchText() + "\\b");
                highlightCursor = editor->document()->find(exp , highlightCursor, flags);
            }else {
                highlightCursor = editor->document()->find(sData.getSearchText() , highlightCursor, flags);
            }
        }
    }
    emit searchRes(sList);
}

void MainWindow::goToStr(int strNum, const SearchData &sData)
{
    QTextBlock bl = editor->document()->findBlockByNumber(strNum);
    unfoldStr(strNum);
    QTextCursor highlightCursor(bl);
    editor->setTextCursor(highlightCursor);
    QTextDocument::FindFlags flags;
    if(sData.getCaseSensivity()) flags |= QTextDocument::FindCaseSensitively;
    if(sData.getWholeWord()) flags |= QTextDocument::FindWholeWords;
    highlightCursor = editor->document()->find(sData.getSearchText(), highlightCursor, flags);
    if (!highlightCursor.isNull()) {
        int blNum = highlightCursor.blockNumber();
        if(blNum==strNum) {
            QTextCursor startPosCursor = highlightCursor;
            highlightCursor.movePosition(QTextCursor::WordRight,
                                QTextCursor::KeepAnchor);
            editor->setTextCursor(startPosCursor);
        }
    }

}

void MainWindow::unfoldStr(int strNum)
{
    QTextBlock block = editor->document()->findBlockByNumber(strNum);
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
}

void MainWindow::replaceTxt(const QString &newTxt)
{
    QTextCursor curs = editor->textCursor();
    if(!curs.selectedText().isEmpty()) {
        curs.insertText(newTxt);
    }
}

void MainWindow::replaceAll(const QString &inp, const QString &out)
{
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start);
    QTextCursor newCursor = cursor;
    quint64 count = 0;
    QTextDocument::FindFlags options;
    options = options | QTextDocument::FindCaseSensitively;
    options = options | QTextDocument::FindWholeWords;
    while (true)
    {
        newCursor = editor->document()->find(inp, newCursor, options);
        if (!newCursor.isNull())
        {
            if (newCursor.hasSelection())
            {
                newCursor.insertText(out);
                count++;
            }
        }else break;
    }
    cursor.endEditBlock();
    QMessageBox::information(this, tr("ReplaceAll"), tr("количество замен - %1").arg(count));
}

void MainWindow::buildPr()
{
    saveFile();
    ui->listWidget->clear();
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
    if(QFile::exists(PathStorage::getBinFileFullName())) {
        debugger->stopDebugger();
        ScanGUI gui(settings->getProgAddr(),true,this);
        int ret = gui.exec();
        if(ret==QDialog::Accepted) {
            BootModeSetter bootSetter(this);
            if(bootSetter.setBootMode()) {
                DetectedController* plc = &DetectedController::Instance();
                YmodemThread loader(plc->getUartName(),this);
                loader.exec();
            }
        }
    }else QMessageBox::warning(this,"Загрузка","Ошибка открытия файла "+PathStorage::getBinFileFullName());
}

void MainWindow::progrAll()
{
    projectToPlc();
    wrSysFramSlot();
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
    ui->listWidget->setCurrentRow(ui->listWidget->count()-1);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(saveWarning()==0) event->ignore();
    else {
        debugger->stopDebugger();
        debugger->closeQuickWatch();
        event->accept();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()) {
    case Qt::Key_F:
        if (QApplication::keyboardModifiers() && Qt::ControlModifier) searchText();
        break;
    case Qt::Key_S:
        if (QApplication::keyboardModifiers() && Qt::ControlModifier) {
            saveFile();
            if(!ui->listWidget->isVisible()) {
                activateInfoPanel();
                ui->listWidget->repaint();
                QThread::sleep(1);
                activateInfoPanel();
            }
        }

        break;
    case Qt::Key_F2:
        if(dockSettings->toggleViewAction()->isEnabled()) {
            dockSettings->setVisible(true);
            dockSettings->raise();
        }
        break;
    case Qt::Key_F3:
        if(dockDisplay->toggleViewAction()->isEnabled()) {
            dockDisplay->setVisible(true);
            dockDisplay->raise();
        }
        break;
    case Qt::Key_F4:
        if(dockDebugger->toggleViewAction()->isEnabled()) {
            dockDebugger->setVisible(true);
            dockDebugger->raise();
        }
        break;
    case Qt::Key_F5:
        if(buildAct->isEnabled()) {
            buildPr();
        }
        break;
    case Qt::Key_F6:
        if(progrAllAct->isEnabled()) {
            progrAll();
        }
        break;
    case Qt::Key_F7:
        if(toPlcAct->isEnabled()) {
            projectToPlc();
        }
        break;
    case Qt::Key_F8:
        //emit wrSysFram();
        wrSysFramSlot();
        break;
    case Qt::Key_F9:
        //emit rdSysFram();
        rdSysFramSlot();
        break;
    }

}

void MainWindow::on_closeInfoListButton_clicked()
{
    ui->listWidget->setVisible(false);
    ui->closeInfoListButton->setVisible(false);
    ui->infoLabel->setVisible(false);
    ui->horizontalSpacer->changeSize(0,0, QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void MainWindow::viewHelp()
{
    helpBr->show();
    helpBr->goHome();
}

void MainWindow::viewAboutWindow()
{
    AboutDialog dialog;
    dialog.exec();
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
        //buildAct->setEnabled(false);
        if(index==2) {
            debugger->tabChanged();
        }
        if(index==3) {
            lcd->updFocus();
            buildAct->setEnabled(true);
        }
    }else {
        undoAct->setEnabled(true);
        redoAct->setEnabled(true);
        srchAct->setEnabled(true);
        buildAct->setEnabled(true);
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
        ui->tabWidget->setTabText(0,"Редактор");
    }
}

void MainWindow::prWasChanged()
{
    prChangedFlag = true;
    ui->tabWidget->setTabText(0,"*Редактор");
}

void MainWindow::buildWithoutErrors()
{
    varOwner->generateVarsTree();

    emit updTree();
    displ->updateDefinitions(*varOwner);
    displ->cursorPosChanged(displ->getXPosition(),displ->getYPosition());
    //QThread::msleep(1000);
    on_closeInfoListButton_clicked();
}

void MainWindow::lcdToTable()
{

    const int startSettingsAddress = 0x7B00;
    const int stopSettingsEndAddress = 0x7EFF;

    QVector<PultVarDefinition> vars;
    displ->getVars(vars);

    if(vars.count()>0) {
        for(int i=0;i<vars.count();i++) {
            QString vID = vars.at(i).getId();
            QString vP = vars.at(i).getPattern();
            VarItem var = varOwner->getVarByID(vID);
            QString memType = var.getMemType();
            if(memType==MemStorage::framMemName) {
                int byteCount = var.getByteCount();
                if(byteCount) {
                    vP.remove(".");
                    vP.remove(",");
                    qulonglong value = (qulonglong)vP.toLongLong();
                    int addr = var.getMemAddress();
                    for(int j=0;j<byteCount;j++) {
                        quint8 vByte = value & 0xFF;
                        value = value >> 8;
                        if((addr>=startSettingsAddress)&&(addr<=stopSettingsEndAddress)) {
                            settings->updateOnyByte(addr-startSettingsAddress,vByte);
                        }
                        addr++;
                    }
                }
            }
        }
        settings->updateTable();
    }
}

void MainWindow::tableToLcd()
{

}

void MainWindow::importPult()
{
    QString fName = getFileNameForPultImport();
    importPult(fName);
}

int MainWindow::importPult(const QString &fName)
{
    if(!fName.isEmpty()) {
        QDomDocument doc("pult");
        QFile file(fName);
        if (!file.open(QIODevice::ReadOnly)) return 0;
        if (!doc.setContent(&file)) {
            return 0;
        }
        QDomNodeList rows = doc.elementsByTagName("Views");
        if(rows.count()==4) {
            displ->clearDisplay();
            LCDPhont ph = lcd->getPhont();
            for(int i=0;i<rows.count();i++) {
                QDomNode n = rows.item(i);
                if(!n.toElement().isNull()) {
                    QDomNodeList views = n.childNodes();
                    for(int j=0;j<views.count();j++) {
                        insertStr(views,i,j,ph);
                    }
                }
                displ->deleteStr(i,0);
            }
            displ->setCursor(0,0);
            displ->clearStack();
            return 1;
        }else {
            QMessageBox::warning(this,"Импортирование пульта","Некорректный формат файла.\n");
        }
    }
    return 0;
}

void MainWindow::importProject()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите каталог проекта"),
                                                    PathStorage::getPrDir(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if(!dir.isEmpty()) {
        QDir oldPr(dir);
        QDir newPr(dir + "/Relkon7_project");
        if(!newPr.exists()) newPr.mkdir(".");

        QStringList konFiles;
        konFiles = oldPr.entryList(QStringList() << "*.kon",
                                         QDir::Files | QDir::NoSymLinks);
        if(konFiles.count()) {
            addMessageToInfoList("копирование kon файла");
            QString konFile = newPr.absolutePath()+"/"+konFiles.at(0);
            QFile::copy(oldPr.absolutePath()+"/"+konFiles.at(0), konFile);
            if(openFileByName(konFile)==1) {
                QStringList pultFiles;
                pultFiles = oldPr.entryList(QStringList() << "*.plt",
                                                 QDir::Files | QDir::NoSymLinks);
                if(pultFiles.count()) {
                    QString pultFile = oldPr.absolutePath()+"/"+pultFiles.at(0);
                    if(importPult(pultFile)) {
                        addMessageToInfoList("импорта пульта");
                        QStringList rp6Files;
                        rp6Files = oldPr.entryList(QStringList() << "*.rp6",
                                                         QDir::Files | QDir::NoSymLinks);
                        if(rp6Files.count()) {
                            QString rp6File = oldPr.absolutePath()+"/"+rp6Files.at(0);
                            if(loadSysFramRelk6(rp6File)) {
                                addMessageToInfoList("импорт заводских настроек");
                                debugger->clearView();
                                buildPr();
                            }else addMessageToInfoList("файл настроек не найден");
                        }
                    }
                }else addMessageToInfoList("файл пульта не найден");
            }
        }else addMessageToInfoList("kon файл не найден");
    }
}

void MainWindow::editIDESettings()
{
    DialogEditGUISettings dialog;
    dialog.exec();
}

void MainWindow::startReloader()
{

    QString path = QApplication::applicationDirPath() + "/Reloader.exe";
    if(QFile::exists(path)) {
        QProcess* loader = new QProcess;
        connect(loader, SIGNAL(finished(int)), loader, SLOT(deleteLater()));
        loader->start(path);
    }else {
        activateInfoPanel();
        addMessageToInfoList("error: Ошибка открытия файла " + path);
    }

}

void MainWindow::startMMBConfig()
{
    QString path = QApplication::applicationDirPath() + "/mmbSettings.exe";
    if(QFile::exists(path)) {
        QProcess* mmb = new QProcess;
        connect(mmb, SIGNAL(finished(int)), mmb, SLOT(deleteLater()));
        mmb->start(path);
    }else {
        activateInfoPanel();
        addMessageToInfoList("error: Ошибка открытия файла " + path);
    }
}

void MainWindow::loadSysFramRelk6()
{
    QStringList prevProjects = getPrevProjects();
    QString path = "/";
    if(prevProjects.count()) {
        if(QFile::exists(prevProjects.at(0))) {
            QFileInfo fInfo(prevProjects.at(0));
            path = fInfo.absolutePath();
        }
    }
    QString fName = QFileDialog::getOpenFileName(this, tr("Импорт зав. установок"),
                                                    path,
                                                    tr("Relkon 6.x project (*.rp6 )"));
    loadSysFramRelk6(fName);
}

int MainWindow::loadSysFramRelk6(const QString &fName)
{
    if(!fName.isEmpty()) {
        QDomDocument doc;
        QFile file(fName);
        if(file.open(QIODevice::ReadOnly)) {

            if(doc.setContent(&file)) {
                bool updFlag = false;
                QDomNodeList vars = doc.elementsByTagName("ControllerVar");
                for(int i=0;i<vars.count();i++) {
                    QDomNode n = vars.item(i);
                    QDomElement e = n.toElement();
                    if(!e.isNull()) {
                        QString name = e.attribute("Name");
                        QString value = e.attribute("Value");
                        QRegExp exp("^EE(\\d+)$");
                        if(exp.indexIn(name)!=-1) {
                            int num = exp.cap(1).toInt();
                            unsigned char ucValue = value.toUInt();
                            if(num<SettingsBase::getCount()) {
                                settings->updateOnyByte(num,ucValue);
                                updFlag = true;
                            }
                        }
                    }
                }
                if(updFlag) settings->updateTable();
                else QMessageBox::information(this,"Сообщение","Не найдено данных для обновления");
            }
            file.close();
            return 1;
        }else QMessageBox::information(this,"Ошибка","Не удалось открыть файл");
    }
    return 0;
}
