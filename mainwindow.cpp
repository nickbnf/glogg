/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
 *
 * This file is part of glogg.
 *
 * glogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * glogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with glogg.  If not, see <http://www.gnu.org/licenses/>.
 */

// This file implements MainWindow. It is responsible for creating and
// managing the menus, the toolbar, and the CrawlerWidget. It also
// load/save the settings on opening/closing of the app

#include <iostream>
#include <QtGui>

#include "log.h"

#include "mainwindow.h"

#include "version.h"
#include "crawlerwidget.h"
#include "filtersdialog.h"
#include "optionsdialog.h"

MainWindow::MainWindow( QString fileName ) : mainIcon_()
{
    // Register the operators for serializable classes
    qRegisterMetaTypeStreamOperators<SavedSearches>( "SavedSearches" );
    qRegisterMetaTypeStreamOperators<Filter>( "Filter" );
    qRegisterMetaTypeStreamOperators<FilterSet>( "FilterSet" );

    createActions();
    createMenus();
    // createContextMenu();
    createToolBars();
    // createStatusBar();
    createCrawler();

    setAcceptDrops( true );

    // Default geometry
    const QRect geometry = QApplication::desktop()->availableGeometry( this );
    setGeometry( geometry.x() + 20, geometry.y() + 40,
            geometry.width() - 140, geometry.height() - 140 );

    connect( this, SIGNAL( optionsChanged() ),
            crawlerWidget, SLOT( applyConfiguration() ));

    readSettings();
    emit optionsChanged();

    // Is there a file passed as argument?
    if ( !fileName.isEmpty() )
        loadFile( fileName );
    else if ( !previousFile.isEmpty() )
        loadFile( previousFile );
    else
        setCurrentFile("", 0, 0);

    mainIcon_.addFile( ":/images/hicolor/16x16/glogg.png" );
    mainIcon_.addFile( ":/images/hicolor/24x24/glogg.png" );
    mainIcon_.addFile( ":/images/hicolor/32x32/glogg.png" );
    mainIcon_.addFile( ":/images/hicolor/48x48/glogg.png" );

    setWindowIcon( mainIcon_ );
    setCentralWidget(crawlerWidget);
}

void MainWindow::createCrawler()
{
    // First create the searches history
    savedSearches = new SavedSearches();

    crawlerWidget = new CrawlerWidget( savedSearches );
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
    infoLine = new InfoLine();
    infoLine->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
    infoLine->setLineWidth( 0 );

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
    connect(&dialog, SIGNAL( optionsChanged() ), crawlerWidget, SLOT( applyConfiguration() ));
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
    QMessageBox::about(this, tr("About glogg"),
            tr("<h2>glogg " GLOGG_VERSION "</h2>"
                "<p>A fast, advanced log explorer."
#ifdef GLOGG_COMMIT
                "<p>Built " GLOGG_DATE " from " GLOGG_COMMIT
#endif
                "<p>Copyright &copy; 2009 Nicolas Bonnefon and other contributors"
                "<p>You may modify and redistribute the program under the terms of the GPL (version 3 or later)." ) );
}

// Opens the 'About Qt' dialog box.
void MainWindow::aboutQt()
{
}

void MainWindow::updateLoadingProgress( int progress )
{
    LOG(logDEBUG) << "Loading progress: " << progress;
    infoLine->setText( tr( "Indexing lines... (%1 %)" ).arg( progress ) );
    infoLine->displayGauge( progress );
    QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );
}

//
// Events
//

// Closes the application
void MainWindow::closeEvent( QCloseEvent *event )
{
    writeSettings();
    event->accept();
}

// Accepts the drag event if it looks like a filename
void MainWindow::dragEnterEvent( QDragEnterEvent* event )
{
    if ( event->mimeData()->hasFormat( "text/uri-list" ) )
        event->acceptProposedAction();
}

// Tries and loads the file if the URL dropped is local
void MainWindow::dropEvent( QDropEvent* event )
{
    QList<QUrl> urls = event->mimeData()->urls();
    if ( urls.isEmpty() )
        return;

    QString fileName = urls.first().toLocalFile();
    if ( fileName.isEmpty() )
        return;

    loadFile( fileName );
}

//
// Private functions
//

// Loads the passed file into the CrawlerWidget and update the title bar.
bool MainWindow::loadFile( const QString& fileName )
{
    int topLine = 0;

    // If we're loading the same file, put the same line on top.
    if ( fileName == currentFile )
        topLine = crawlerWidget->getTopLine();

    // Register for progress status bar
    connect( crawlerWidget, SIGNAL( loadingProgressed( int ) ),
            this, SLOT( updateLoadingProgress( int ) ) );

    // Load the file
    updateLoadingProgress( 0 );
    if ( crawlerWidget->readFile( fileName, topLine ) ) {
        LOG(logDEBUG) << "Success loading file " << fileName.toStdString();
        int fileSize, fileNbLine;
        crawlerWidget->getFileInfo( &fileSize, &fileNbLine );
        setCurrentFile( fileName, fileSize, fileNbLine );
        disconnect( crawlerWidget, SIGNAL( loadingProgressed( int ) ), 0, 0 );
        infoLine->hideGauge();
        return true;
    }
    else {
        LOG(logWARNING) << "Cannot load file " << fileName.toStdString();
        disconnect( crawlerWidget, SIGNAL( loadingProgressed( int ) ), 0, 0 );
        infoLine->hideGauge();
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
    }
    else {
        infoLine->setText( "" );
    }

    setWindowTitle(tr("%1 - %2").arg(shownName).arg(tr("glogg")));
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
    QSettings settings( "glogg", "glogg" );

    // Geometry of us and crawlerWidget (splitter pos, etc...)
    settings.setValue( "geometry", saveGeometry() );
    settings.setValue( "crawlerWidget", crawlerWidget->saveState() );

    // Current file and history
    settings.setValue( "currentFile", currentFile );
    settings.setValue( "recentFiles", recentFiles );

    // Searches history
    settings.setValue( "savedSearches", QVariant::fromValue( *savedSearches ) );

    // User settings
    Config().writeToSettings( settings );
}

// Read settings from permanent storage
// It uses Qt settings storage.
void MainWindow::readSettings()
{
    QSettings settings( "glogg", "glogg" );

    restoreGeometry( settings.value("geometry").toByteArray() );
    crawlerWidget->restoreState( settings.value("crawlerWidget").toByteArray() );

    recentFiles = settings.value("recentFiles").toStringList();
    previousFile = settings.value("currentFile").toString();

    updateRecentFileActions();

    // Copy the searches from the config file to our list
    *savedSearches = settings.value( "savedSearches" ).value<SavedSearches>();

    Config().readFromSettings( settings );
}
