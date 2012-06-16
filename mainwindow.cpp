/*
 * Copyright (C) 2009, 2010, 2011 Nicolas Bonnefon and other contributors
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

#include "sessioninfo.h"
#include "recentfiles.h"
#include "crawlerwidget.h"
#include "filtersdialog.h"
#include "optionsdialog.h"
#include "persistentinfo.h"
#include "savedsearches.h"
#include "menuactiontooltipbehavior.h"

MainWindow::MainWindow() :
    recentFiles( Persistent<RecentFiles>( "recentFiles" ) ), mainIcon_()
{
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

    // Send actions to the crawlerwidget
    connect( this, SIGNAL( followSet( bool ) ),
            crawlerWidget, SIGNAL( followSet( bool ) ) );
    connect( this, SIGNAL( optionsChanged() ),
            crawlerWidget, SLOT( applyConfiguration() ) );

    // Actions from the CrawlerWidget
    connect( crawlerWidget, SIGNAL( followDisabled() ),
            this, SLOT( disableFollow() ) );
    connect( crawlerWidget, SIGNAL( updateLineNumber( int ) ),
            this, SLOT( lineNumberHandler( int ) ) );

    readSettings();
    emit optionsChanged();

    // We start with the empty file
    setCurrentFile( "" );

    mainIcon_.addFile( ":/images/hicolor/16x16/glogg.png" );
    mainIcon_.addFile( ":/images/hicolor/24x24/glogg.png" );
    mainIcon_.addFile( ":/images/hicolor/32x32/glogg.png" );
    mainIcon_.addFile( ":/images/hicolor/48x48/glogg.png" );

    // Register for progress status bar
    connect( crawlerWidget, SIGNAL( loadingProgressed( int ) ),
            this, SLOT( updateLoadingProgress( int ) ) );
    connect( crawlerWidget, SIGNAL( loadingFinished( bool ) ),
            this, SLOT( displayNormalStatus( bool ) ) );

    setWindowIcon( mainIcon_ );
    setCentralWidget(crawlerWidget);
}

void MainWindow::loadInitialFile( QString fileName )
{
    LOG(logDEBUG) << "loadInitialFile";

    // Is there a file passed as argument?
    if ( !fileName.isEmpty() )
        loadFile( fileName );
    else if ( !previousFile.isEmpty() )
        loadFile( previousFile );
}

//
// Private functions
//

void MainWindow::createCrawler()
{
    // First get the global search history
    savedSearches = &(Persistent<SavedSearches>( "savedSearches" ));

    crawlerWidget = new CrawlerWidget( savedSearches );
}

// Menu actions
void MainWindow::createActions()
{
    Configuration& config = Persistent<Configuration>( "settings" );

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
    connect( exitAction, SIGNAL(triggered()), this, SLOT(close()) );

    copyAction = new QAction(tr("&Copy"), this);
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setStatusTip(tr("Copy the selection"));
    connect( copyAction, SIGNAL(triggered()), this, SLOT(copy()) );

    findAction = new QAction(tr("&Find..."), this);
    findAction->setShortcut(QKeySequence::Find);
    findAction->setStatusTip(tr("Find the text"));
    connect( findAction, SIGNAL(triggered()),
            this, SLOT( find() ) );

    overviewVisibleAction = new QAction( tr("Matches &overview"), this );
    overviewVisibleAction->setCheckable( true );
    overviewVisibleAction->setChecked( config.isOverviewVisible() );
    connect( overviewVisibleAction, SIGNAL( toggled( bool ) ),
            this, SLOT( toggleOverviewVisibility( bool )) );

    lineNumbersVisibleAction = new QAction( tr("Line &numbers"), this );
    lineNumbersVisibleAction->setCheckable( true );
    lineNumbersVisibleAction->setChecked( config.lineNumbersVisible() );
    connect( lineNumbersVisibleAction, SIGNAL( toggled( bool ) ),
            this, SLOT( toggleLineNumbersVisibility( bool )) );

    followAction = new QAction( tr("&Follow File"), this );
    followAction->setShortcut(Qt::Key_F);
    followAction->setCheckable(true);
    connect( followAction, SIGNAL(toggled( bool )),
            this, SIGNAL(followSet( bool )) );

    reloadAction = new QAction( tr("&Reload"), this );
    reloadAction->setShortcut(QKeySequence::Refresh);
    reloadAction->setIcon( QIcon(":/images/reload16.png") );
    connect( reloadAction, SIGNAL(triggered()), this, SLOT(reload()) );

    stopAction = new QAction( tr("&Stop"), this );
    stopAction->setIcon( QIcon(":/images/stop16.png") );
    stopAction->setEnabled( false );
    connect( stopAction, SIGNAL(triggered()), this, SLOT(stop()) );

    filtersAction = new QAction(tr("&Filters..."), this);
    filtersAction->setStatusTip(tr("Show the Filters box"));
    connect( filtersAction, SIGNAL(triggered()), this, SLOT(filters()) );

    optionsAction = new QAction(tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Show the Options box"));
    connect( optionsAction, SIGNAL(triggered()), this, SLOT(options()) );

    aboutAction = new QAction(tr("&About"), this);
    aboutAction->setStatusTip(tr("Show the About box"));
    connect( aboutAction, SIGNAL(triggered()), this, SLOT(about()) );

    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutAction->setStatusTip(tr("Show the Qt library's About box"));
    connect( aboutQtAction, SIGNAL(triggered()), this, SLOT(aboutQt()) );
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu( tr("&File") );
    fileMenu->addAction( openAction );
    fileMenu->addSeparator();
    for (int i = 0; i < MaxRecentFiles; ++i) {
        fileMenu->addAction( recentFileActions[i] );
        recentFileActionBehaviors[i] =
            new MenuActionToolTipBehavior(recentFileActions[i], fileMenu, this);
    }
    fileMenu->addSeparator();
    fileMenu->addAction( exitAction );

    editMenu = menuBar()->addMenu( tr("&Edit") );
    editMenu->addAction( copyAction );
    editMenu->addAction( findAction );

    viewMenu = menuBar()->addMenu( tr("&View") );
    viewMenu->addAction( overviewVisibleAction );
    viewMenu->addAction( lineNumbersVisibleAction );
    viewMenu->addSeparator();
    viewMenu->addAction( followAction );
    viewMenu->addSeparator();
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

    lineNbField = new QLabel( );
    lineNbField->setText( "Line 0" );
    lineNbField->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    lineNbField->setMinimumSize(
            lineNbField->fontMetrics().size( 0, "Line 0000000") );

    toolBar = addToolBar( tr("&Toolbar") );
    toolBar->setIconSize( QSize( 16, 16 ) );
    toolBar->setMovable( false );
    toolBar->addAction( openAction );
    toolBar->addAction( reloadAction );
    toolBar->addWidget( infoLine );
    toolBar->addAction( stopAction );
    toolBar->addWidget( lineNbField );
}

//
// Slots
//

// Opens the file selection dialog to select a new log file
void MainWindow::open()
{
    QString defaultDir = ".";

    // Default to the path of the current file if there is one
    if ( !currentFile.isEmpty() ) {
        QFileInfo fileInfo = QFileInfo( currentFile );
        defaultDir = fileInfo.path();
    }

    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open file"), defaultDir, tr("All files (*)"));
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
    static QClipboard* clipboard = QApplication::clipboard();

    clipboard->setText( crawlerWidget->getSelectedText() );

    // Put it in the global selection as well (X11 only)
    clipboard->setText( crawlerWidget->getSelectedText(),
            QClipboard::Selection );
}

void MainWindow::find()
{
    crawlerWidget->displayQuickFindBar();
}

// Reload the current log file
void MainWindow::reload()
{
    if ( !currentFile.isEmpty() )
        loadFile( currentFile );
}

// Stop the loading operation
void MainWindow::stop()
{
    crawlerWidget->stopLoading();
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
                "<p>Copyright &copy; 2009, 2010, 2011 Nicolas Bonnefon and other contributors"
                "<p>You may modify and redistribute the program under the terms of the GPL (version 3 or later)." ) );
}

// Opens the 'About Qt' dialog box.
void MainWindow::aboutQt()
{
}

void MainWindow::toggleOverviewVisibility( bool isVisible )
{
    Configuration& config = Persistent<Configuration>( "settings" );
    config.setOverviewVisible( isVisible );
    emit optionsChanged();
}

void MainWindow::toggleLineNumbersVisibility( bool isVisible )
{
    Configuration& config = Persistent<Configuration>( "settings" );
    config.setLineNumbersVisible( isVisible );
    emit optionsChanged();
}

void MainWindow::disableFollow()
{
    followAction->setChecked( false );
}

void MainWindow::lineNumberHandler( int line )
{
    // The line number received is the internal (starts at 0)
    lineNbField->setText( tr( "Line %1" ).arg( line + 1 ) );
}

void MainWindow::updateLoadingProgress( int progress )
{
    LOG(logDEBUG) << "Loading progress: " << progress;

    // We ignore 0% and 100% to avoid a flash when the file (or update)
    // is very short.
    if ( progress > 0 && progress < 100 ) {
        infoLine->setText( loadingFileName + tr( " - Indexing lines... (%1 %)" ).arg( progress ) );
        infoLine->displayGauge( progress );

        stopAction->setEnabled( true );
    }
}

void MainWindow::displayNormalStatus( bool success )
{
    QLocale defaultLocale;

    LOG(logDEBUG) << "displayNormalStatus";

    if ( success )
        setCurrentFile( loadingFileName );

    qint64 fileSize;
    int fileNbLine;
    QDateTime lastModified;

    crawlerWidget->getFileInfo( &fileSize, &fileNbLine, &lastModified );
    if ( lastModified.isValid() ) {
        const QString date =
#if QT_VERSION > 0x040500
            defaultLocale.toString( lastModified, QLocale::NarrowFormat );
#else
            defaultLocale.toString( lastModified.date(), QLocale::ShortFormat )
                .append( " " ).append( defaultLocale.toString(
                            lastModified.time(), QLocale::ShortFormat ) );
#endif
        infoLine->setText( tr( "%1 (%2 - %3 lines - modified on %4)" )
                .arg(currentFile).arg(readableSize(fileSize))
                .arg(fileNbLine).arg( date ) );
    }
    else {
        infoLine->setText( tr( "%1 (%2 - %3 lines)" )
                .arg(currentFile).arg(readableSize(fileSize))
                .arg(fileNbLine) );
    }

    infoLine->hideGauge();
    stopAction->setEnabled( false );
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
// The loading is done asynchronously.
bool MainWindow::loadFile( const QString& fileName )
{
    LOG(logDEBUG) << "loadFile ( " << fileName.toStdString() << " )";

    int topLine = 0;

    // If we're loading the same file, put the same line on top.
    if ( fileName == currentFile )
        topLine = crawlerWidget->getTopLine();

    // Load the file
    loadingFileName = fileName;
    if ( crawlerWidget->readFile( fileName, topLine ) ) {
        LOG(logDEBUG) << "Success loading file " << fileName.toStdString();
        return true;
    }
    else {
        LOG(logWARNING) << "Cannot load file " << fileName.toStdString();
        displayNormalStatus( false );
        return false;
    }
}

// Strips the passed filename from its directory part.
QString MainWindow::strippedName( const QString& fullFileName ) const
{
    return QFileInfo( fullFileName ).fileName();
}

// Add the filename to the recent files list and update the title bar.
void MainWindow::setCurrentFile( const QString& fileName )
{
    // Change the current file
    currentFile = fileName;
    QString shownName = tr( "Untitled" );
    if ( !currentFile.isEmpty() ) {
        // (reload the list first in case another glogg changed it)
        GetPersistentInfo().retrieve( "recentFiles" );
        recentFiles.addRecent( currentFile );
        GetPersistentInfo().save( "recentFiles" );
        updateRecentFileActions();
        shownName = strippedName( currentFile );
    }

    setWindowTitle(
            tr("%1 - %2").arg(shownName).arg(tr("glogg"))
#ifdef GLOGG_COMMIT
            + " (dev build " GLOGG_VERSION ")"
#endif
            );
}

// Updates the actions for the recent files.
// Must be called after having added a new name to the list.
void MainWindow::updateRecentFileActions()
{
    QStringList recent_files = recentFiles.recentFiles();

    for ( int j = 0; j < MaxRecentFiles; ++j ) {
        if ( j < recent_files.count() ) {
            QString text = tr("&%1 %2").arg(j + 1).arg(strippedName(recent_files[j]));
            recentFileActions[j]->setText( text );
            recentFileActions[j]->setToolTip( recent_files[j] );
            recentFileActions[j]->setData( recent_files[j] );
            recentFileActions[j]->setVisible( true );
        }
        else {
            recentFileActions[j]->setVisible( false );
        }
    }

    // separatorAction->setVisible(!recentFiles.isEmpty());
}

// Write settings to permanent storage
void MainWindow::writeSettings()
{
    // Save the session
    SessionInfo& session = Persistent<SessionInfo>( "session" );
    session.setGeometry( saveGeometry() );
    session.setCrawlerState( crawlerWidget->saveState() );
    session.setCurrentFile( currentFile );
    GetPersistentInfo().save( QString( "session" ) );

    // User settings
    GetPersistentInfo().save( QString( "settings" ) );
}

// Read settings from permanent storage
void MainWindow::readSettings()
{
    // Get and restore the session
    GetPersistentInfo().retrieve( QString( "session" ) );
    SessionInfo session = Persistent<SessionInfo>( "session" );
    restoreGeometry( session.geometry() );
    crawlerWidget->restoreState( session.crawlerState() );
    previousFile = session.currentFile();

    // History of recent files
    GetPersistentInfo().retrieve( QString( "recentFiles" ) );
    updateRecentFileActions();

    GetPersistentInfo().retrieve( QString( "savedSearches" ) );
    GetPersistentInfo().retrieve( QString( "settings" ) );
    GetPersistentInfo().retrieve( QString( "filterSet" ) );
}

// Returns the size in human readable format
QString MainWindow::readableSize( qint64 size ) const
{
    static const QString sizeStrs[] = {
        tr("B"), tr("KiB"), tr("MiB"), tr("GiB"), tr("TiB") };

    QLocale defaultLocale;
    unsigned int i;
    double humanSize = size;

    for ( i=0; i+1 < (sizeof(sizeStrs)/sizeof(QString)) && (humanSize/1024.0) >= 1024.0; i++ )
        humanSize /= 1024.0;

    if ( humanSize >= 1024.0 ) {
        humanSize /= 1024.0;
        i++;
    }

    QString output;
    if ( i == 0 )
        // No decimal part if we display straight bytes.
        output = defaultLocale.toString( (int) humanSize );
    else
        output = defaultLocale.toString( humanSize, 'f', 1 );

    output += QString(" ") + sizeStrs[i];

    return output;
}
