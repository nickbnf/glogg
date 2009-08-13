#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "crawlerwidget.h"

class QAction;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

protected:
    void closeEvent(QCloseEvent *event);
private slots:
    void open();
    void openRecentFile();
    void about();
    void aboutQt();

    void updateStatusBar();

private:
    void createActions();
    void createMenus();
    void createContextMenu();
    void createToolBars();
    void createStatusBar();
    void createCrawler();
    void readSettings();
    void writeSettings();
    bool loadFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    void updateRecentFileActions();
    QString strippedName(const QString &fullFileName);

    CrawlerWidget *crawlerWidget;
    QStringList recentFiles;
    QString currentFile;

    enum { MaxRecentFiles = 5 };
    QAction *recentFileActions[MaxRecentFiles];
    QAction *separatorAction;

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *helpMenu;

    QToolBar *fileToolBar;
    QToolBar *editToolBar;

    QAction *openAction;
    QAction *exitAction;
    QAction *aboutAction;
    QAction *aboutQtAction;
};

#endif
