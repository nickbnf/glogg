#include <iostream>
#include <QtGui>

#include "log.h"

#include "mainwindow.h"

#include "version.h"
#include "crawlerwidget.h"

MainWindow::MainWindow()
{

    createActions();
    createMenus();
    // createContextMenu();
    // createToolBars();
    // createStatusBar();
    createCrawler();

    setCurrentFile("");
    readSettings();

    setWindowIcon(QIcon(":/images/icon.png"));
    setCentralWidget(crawlerWidget);
}

void MainWindow::createCrawler()
{
    crawlerWidget = new CrawlerWidget;
}

void MainWindow::createActions()
{
    openAction = new QAction(tr("&Open..."), this);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setStatusTip(tr("Open a file"));
    connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActions[i] = new QAction(this);
        recentFileActions[i]->setVisible(false);
        connect(recentFileActions[i], SIGNAL(triggered()),
                this, SLOT(openRecentFile()));
    }

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(tr("Ctrl+Q"));
    exitAction->setStatusTip(tr("Exit the application"));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    aboutAction = new QAction(tr("&About"), this);
    aboutAction->setStatusTip(tr("Show the About box"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutAction->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAction, SIGNAL(triggered()), this, SLOT(aboutQt()));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAction);
    fileMenu->addSeparator();
    for (int i = 0; i < MaxRecentFiles; ++i)
        fileMenu->addAction(recentFileActions[i]);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
}

/*
 * Slots
 */
void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open file"), ".", tr("All files (*)"));
    if (!fileName.isEmpty())
        loadFile(fileName);
}

void MainWindow::openRecentFile()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
        loadFile(action->data().toString());
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About LogCrawler"),
            tr("<h2>LogCrawler " LCRAWLER_VERSION "</h2>"
                "<p>Copyright &copy; 2009 Nicolas Bonnefon"
                "<p>A fast, advanced log explorer."
                "<p>Built " LCRAWLER_DATE));
}

void MainWindow::aboutQt()
{
}

void MainWindow::updateStatusBar()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

/*
 * Private functions
 */
bool MainWindow::loadFile(const QString &fileName)
{
    if (crawlerWidget->readFile(fileName)) {
        LOG(logDEBUG) << "Success loading file " << fileName.toStdString();
        setCurrentFile(fileName);
        return true;
    }
    else {
        LOG(logWARNING) << "Cannot load file " << fileName.toStdString();
        return false;
    }
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    currentFile = fileName;
    QString shownName = tr("Untitled");
    if (!currentFile.isEmpty()) {
        shownName = strippedName(currentFile);
        recentFiles.removeAll(currentFile);
        recentFiles.prepend(currentFile);
        updateRecentFileActions();
    }

    setWindowTitle(tr("%1 - %2").arg(shownName).arg(tr("LogCrawler")));
}

void MainWindow::updateRecentFileActions()
{
    // First remove non existent files
    QMutableStringListIterator i(recentFiles);
    while (i.hasNext()) {
        if (!QFile::exists(i.next()))
            i.remove();
    }

    for (int j = 0; j < MaxRecentFiles; ++j) {
        if (j < recentFiles.count()) {
            QString text = tr("&%1 %2").arg(j + 1).arg(strippedName(recentFiles[j]));
            recentFileActions[j]->setText(text);
            recentFileActions[j]->setData(recentFiles[j]);
            recentFileActions[j]->setVisible(true);
        }
        else {
            recentFileActions[j]->setVisible(false);
        }
    }

    // separatorAction->setVisible(!recentFiles.isEmpty());
}

void MainWindow::writeSettings()
{
    QSettings settings("LogCrawler", "LogCrawler");

    settings.setValue("geometry", saveGeometry());
    settings.setValue("currentFile", currentFile);
    settings.setValue("recentFiles", recentFiles);
}

void MainWindow::readSettings()
{
    QSettings settings("LogCrawler", "LogCrawler");

    restoreGeometry(settings.value("geometry").toByteArray());

    recentFiles = settings.value("recentFiles").toStringList();
    QString curFile = settings.value("currentFile").toString();
    if (!curFile.isEmpty())
        loadFile(curFile);

    updateRecentFileActions();
}
