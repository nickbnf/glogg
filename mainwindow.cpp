/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
 *
 * This file is part of LogCrawler.
 *
 * LogCrawler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LogCrawler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LogCrawler.  If not, see <http://www.gnu.org/licenses/>.
 */

// This file implements MainWindow. It is responsible for creating and
// managing the menus, the toolbar, and the CrawlerWidget. It also
// load/save the settings on opening/closing of the app
// It keeps track of the file on disk.

#include <iostream>
#include <QtGui>

#include "log.h"

#include "mainwindow.h"

#include "version.h"
#include "crawlerwidget.h"
#include "filtersdialog.h"
#include "optionsdialog.h"

MainWindow::MainWindow() : fileWatcher( this )
{
    createActions();
    createMenus();
    // createContextMenu();
    createToolBars();
    // createStatusBar();
    createCrawler();

    // Initialize the file watcher
    connect( &fileWatcher, SIGNAL( fileChanged( const QString& ) ),
            this, SLOT( signalFileChanged( const QString& ) ) );

    connect( this, SIGNAL( optionsChanged() ),
            crawlerWidget, SLOT( applyConfiguration() ));
    setCurrentFile("", 0, 0);
    readSettings();
    emit optionsChanged();

    setWindowIcon(QIcon(":/images/logcrawler.png"));
    setCentralWidget(crawlerWidget);
}

void MainWindow::createCrawler()
{
    crawlerWidget = new CrawlerWidget;
}

// Menu actions
void MainWindow::createActions()
{
    openAction = new QAction(tr("&Open..."), this);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setIcon( QIcon(":/images/open16.png") );
    openAction->setStatusTip(tr("Open a file"));
    connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

    // Recent files
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

    copyAction = new QAction(tr("&Copy"), this);
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setStatusTip(tr("Copy the selected line"));
    connect(copyAction, SIGNAL(triggered()), this, SLOT(copy()));

    reloadAction = new QAction( tr("&Reload"), this );
    reloadAction->setIcon( QIcon(":/images/reload16.png") );
    connect( reloadAction, SIGNAL(triggered()), this, SLOT(reload()) );

    filtersAction = new QAction(tr("&Filters..."), this);
    filtersAction->setStatusTip(tr("Show the Filters box"));
    connect(filtersAction, SIGNAL(triggered()), this, SLOT(filters()));

    optionsAction = new QAction(tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Show the Options box"));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(options()));

    aboutAction = new QAction(tr("&About"), this);
    aboutAction->setStatusTip(tr("Show the About box"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutAction->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAction, SIGNAL(triggered()), this, SLOT(aboutQt()));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu( tr("&File") );
    fileMenu->addAction( openAction );
    fileMenu->addSeparator();
    for (int i = 0; i < MaxRecentFiles; ++i)
        fileMenu->addAction( recentFileActions[i] );
    fileMenu->addSeparator();
    fileMenu->addAction( exitAction );

    editMenu = menuBar()->addMenu( tr("&Edit") );
    editMenu->addAction( copyAction );

    viewMenu = menuBar()->addMenu( tr("&View") );
    viewMenu->addAction( reloadAction );

    toolsMenu = menuBar()->addMenu( tr("&Tools") );
    toolsMenu->addAction( filtersAction );
    toolsMenu->addSeparator();
    toolsMenu->addAction( optionsAction );

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu( tr("&Help") );
    helpMenu->addAction( aboutAction );
}

void MainWindow::createToolBars()
{
    infoLine = new QLabel();
    infoLine->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
    infoLine->setLineWidth( 0 );
    infoLine->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    toolBar = addToolBar( tr("&Toolbar") );
    toolBar->setIconSize( QSize( 16, 16 ) );
    toolBar->setMovable( false );
    toolBar->addAction( openAction );
    toolBar->addAction( reloadAction );
    toolBar->addWidget( infoLine );
}

//
// Slots
//

// Opens the file selection dialog to select a new log file
void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open file"), ".", tr("All files (*)"));
    if (!fileName.isEmpty())
        loadFile(fileName);
}

// Opens a log file from the recent files list
void MainWindow::openRecentFile()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
        loadFile(action->data().toString());
}

// Copy the currently selected line into the clipboard
void MainWindow::copy()
{
}

// Reload the current log file
void MainWindow::reload()
{
    if ( !currentFile.isEmpty() )
        loadFile( currentFile );
}

// Opens the 'Filters' dialog box
void MainWindow::filters()
{
    FiltersDialog dialog(this);
    // connect(&dialog, SIGNAL( optionsChanged() ), crawlerWidget, SLOT( applyConfiguration() ));
    dialog.exec();
}

// Opens the 'Options' modal dialog box
void MainWindow::options()
{
    OptionsDialog dialog(this);
    connect(&dialog, SIGNAL( optionsChanged() ), crawlerWidget, SLOT( applyConfiguration() ));
    dialog.exec();
}

// Opens the 'About' dialog box.
void MainWindow::about()
{
    QMessageBox::about(this, tr("About LogCrawler"),
            tr("<h2>LogCrawler " LCRAWLER_VERSION "</h2>"
                "<p>A fast, advanced log explorer."
                "<p>Built " LCRAWLER_DATE " from " LCRAWLER_COMMIT
                "<p>Copyright &copy; 2009 Nicolas Bonnefon and other contributors"
                "<p>You may modify and redistribute the program under the terms of the GPL (version 3 or later)." ) );
}

// Opens the 'About Qt' dialog box.
void MainWindow::aboutQt()
{
}

void MainWindow::signalFileChanged( const QString& fileName )
{
    LOG(logDEBUG) << "signalFileChanged";
    if ( !fileChangedOnDisk ) {
        fileChangedOnDisk = true;
        infoLine->setText( infoLine->text() + tr(" - file changed on disk") );
    }
}

// Closes the application
void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

//
// Private functions
//

// Loads the passed file into the CrawlerWidget and update the title bar.
bool MainWindow::loadFile( const QString& fileName )
{
    if ( crawlerWidget->readFile( fileName ) ) {
        LOG(logDEBUG) << "Success loading file " << fileName.toStdString();
        int fileSize, fileNbLine;
        crawlerWidget->getFileInfo( &fileSize, &fileNbLine );
        setCurrentFile( fileName, fileSize, fileNbLine );
        return true;
    }
    else {
        LOG(logWARNING) << "Cannot load file " << fileName.toStdString();
        return false;
    }
}

// Strips the passed filename from its directory part.
QString MainWindow::strippedName( const QString& fullFileName )
{
    return QFileInfo( fullFileName ).fileName();
}

// Add the filename to the recent files list and update the title bar.
void MainWindow::setCurrentFile( const QString& fileName,
        int fileSize, int fileNbLine )
{
    // Remove the current file from the watch list
    fileWatcher.removePath( currentFile );

    // Change the current file
    currentFile = fileName;
    QString shownName = tr( "Untitled" );
    if ( !currentFile.isEmpty() ) {
        shownName = strippedName( currentFile );
        recentFiles.removeAll( currentFile );
        recentFiles.prepend( currentFile );
        updateRecentFileActions();

        infoLine->setText( tr( "%1 (%2 KB - %3 lines)" )
                    .arg(currentFile).arg(fileSize/1024).arg(fileNbLine) );

        // Watch the new file on disk
        fileChangedOnDisk = false;
        fileWatcher.addPath( currentFile );
    }
    else {
        infoLine->setText( "" );
    }

    setWindowTitle(tr("%1 - %2").arg(shownName).arg(tr("LogCrawler")));
}

// Trims the recent file lists and updates the actions.
// Must be called after having added a new name to the list.
void MainWindow::updateRecentFileActions()
{
    // First remove non existent files
    QMutableStringListIterator i(recentFiles);
    while ( i.hasNext() ) {
        if ( !QFile::exists(i.next()) )
            i.remove();
    }

    for ( int j = 0; j < MaxRecentFiles; ++j ) {
        if ( j < recentFiles.count() ) {
            QString text = tr("&%1 %2").arg(j + 1).arg(strippedName(recentFiles[j]));
            recentFileActions[j]->setText( text );
            recentFileActions[j]->setData( recentFiles[j] );
            recentFileActions[j]->setVisible( true );
        }
        else {
            recentFileActions[j]->setVisible( false );
        }
    }

    // separatorAction->setVisible(!recentFiles.isEmpty());
}

// Write settings to permanent storage
// It uses Qt settings storage.
void MainWindow::writeSettings()
{
    QSettings settings( "LogCrawler", "LogCrawler" );

    // Geometry of us and crawlerWidget (splitter pos, etc...)
    settings.setValue( "geometry", saveGeometry() );
    settings.setValue( "crawlerWidget", crawlerWidget->saveState() );

    // Current file and history
    settings.setValue( "currentFile", currentFile );
    settings.setValue( "recentFiles", recentFiles );

    // User settings
    Config().writeToSettings( settings );
}

// Read settings from permanent storage
// It uses Qt settings storage.
void MainWindow::readSettings()
{
    QSettings settings( "LogCrawler", "LogCrawler" );

    restoreGeometry( settings.value("geometry").toByteArray() );
    crawlerWidget->restoreState( settings.value("crawlerWidget").toByteArray() );

    recentFiles = settings.value("recentFiles").toStringList();
    QString curFile = settings.value("currentFile").toString();
    if (!curFile.isEmpty())
        loadFile(curFile);

    updateRecentFileActions();

    Config().readFromSettings( settings );
}
