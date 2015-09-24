#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "codeeditor.h"
#include <QLineEdit>
#include <QThread>
#include "settingsbase.h"
#include "Debugger/debuggerform.h"
#include "LCD/display.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    CodeEditor *editor;
    SettingsBase *settings;
    DebuggerForm *debugger;
    Display *displ;
    QThread builderThread;

    const QString wTitle = "RelkonIDE ";
    const int maxAmountOfPrevProjects = 10;
    QString prDirPath;
    QString prFileName;

    QStringList getPrevProjects(void);
    void updatePrevProjects(const QStringList &prNames);
    int openFileByName(const QString &fName);
    void saveFileByName(const QString &fName);
    void activateInfoPanel(void);
    int saveWarning(void);

    bool prChangedFlag = false;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void startBuild(QString prPath, QString prName);
    void updateKonFileForBuilder(QStringList str);

private:
    Ui::MainWindow *ui;
    QLineEdit* textForSearch;
    QAction* undoAct;
    QAction* redoAct;
    QAction* srchAct;
    QAction* buildAct;
    QAction* toPlcAct;
    QAction* newAct;
    QAction* openAct;
    QAction* saveAct;
    QAction* saveAsAct;

private slots:
    void newFile(void);
    void openFile(void);
    void saveFile(void);
    void saveAsFile(void);
    void undo(void);
    void redo(void);
    void searchText(void);
    void buildPr(void);
    void projectToPlc(void);
    void on_closeInfoListButton_clicked();

    void on_listWidget_doubleClicked(const QModelIndex &index);

    void on_tabWidget_currentChanged(int index);
    void openPrevProject(void);
    void prWasChanged(void);
    void buildWithoutErrors(void);

public slots:
    void addMessageToInfoList(const QString &message);

protected:
     void closeEvent(QCloseEvent *event);
};

#endif // MAINWINDOW_H
