#include <QtGui>

#include "mainwindow.h"

#include "crawler.h"

MainWindow::MainWindow()
{
    crawler = new Crawler;
    setCentralWidget(crawler);

    createActions();
    createMenus();
    // createContextMenu();
    createToolBars();
    createStatusBar();

    readSettings();

    findDialog = 0;

    setWindowIcon(QIcon(":/images/icon.png"));
    setCurrentFile("");
}
