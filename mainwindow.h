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
    void copy();
    void options();
    void about();
    void aboutQt();

    void updateStatusBar();

signals:
    /// Is emitted when new settings must be used
    void optionsChanged();

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
    QMenu *toolsMenu;
    QMenu *helpMenu;

    QAction *openAction;
    QAction *exitAction;
    QAction *copyAction;
    QAction *optionsAction;
    QAction *aboutAction;
    QAction *aboutQtAction;
};

#endif
