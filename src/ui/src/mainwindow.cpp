/*
 * Copyright (C) 2009, 2010, 2011, 2013, 2014 Nicolas Bonnefon and other contributors
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
#include <cassert>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32


#include <QAction>
#include <QDesktopWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QFileInfo>
#include <QFileDialog>
#include <QClipboard>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>

#include "log.h"

#include "mainwindow.h"

#include "sessioninfo.h"
#include "recentfiles.h"
#include "crawlerwidget.h"
#include "filtersdialog.h"
#include "optionsdialog.h"
#include "persistentinfo.h"
#include "menuactiontooltipbehavior.h"
#include "tabbedcrawlerwidget.h"
#include "externalcom.h"

#include "version.h"

// Returns the size in human readable format
static QString readableSize( qint64 size );

MainWindow::MainWindow( std::unique_ptr<Session> session,
        std::shared_ptr<ExternalCommunicator> external_communicator ) :
    session_( std::move( session )  ),
    externalCommunicator_( external_communicator ),
    recentFiles_( Persistent<RecentFiles>( "recentFiles" ) ),
    mainIcon_(),
    signalMux_(),
    quickFindMux_( session_->getQuickFindPattern() ),
    mainTabWidget_()
#ifdef GLOGG_SUPPORTS_VERSION_CHECKING
    ,versionChecker_()
#endif
{
    createActions();
    createMenus();
    createToolBars();
    // createStatusBar();

    setAcceptDrops( true );

    // Default geometry
    const QRect geometry = QApplication::desktop()->availableGeometry( this );
    setGeometry( geometry.x() + 20, geometry.y() + 40,
            geometry.width() - 140, geometry.height() - 140 );

    mainIcon_.addFile( ":/images/hicolor/16x16/glogg.png" );
    mainIcon_.addFile( ":/images/hicolor/24x24/glogg.png" );
    mainIcon_.addFile( ":/images/hicolor/32x32/glogg.png" );
    mainIcon_.addFile( ":/images/hicolor/48x48/glogg.png" );

    setWindowIcon( mainIcon_ );

    readSettings();

    // Connect the signals to the mux (they will be forwarded to the
    // "current" crawlerwidget

    // Send actions to the crawlerwidget
    signalMux_.connect( this, SIGNAL( followSet( bool ) ),
            SIGNAL( followSet( bool ) ) );
    signalMux_.connect( this, SIGNAL( optionsChanged() ),
            SLOT( applyConfiguration() ) );
    signalMux_.connect( this, SIGNAL( enteringQuickFind() ),
            SLOT( enteringQuickFind() ) );
    signalMux_.connect( &quickFindWidget_, SIGNAL( close() ),
            SLOT( exitingQuickFind() ) );

    // Actions from the CrawlerWidget
    signalMux_.connect( SIGNAL( followModeChanged( bool ) ),
            this, SLOT( changeFollowMode( bool ) ) );
    signalMux_.connect( SIGNAL( updateLineNumber( LineNumber ) ),
            this, SLOT( lineNumberHandler( LineNumber ) ) );

    // Register for progress status bar
    signalMux_.connect( SIGNAL( loadingProgressed( int ) ),
            this, SLOT( updateLoadingProgress( int ) ) );
    signalMux_.connect( SIGNAL( loadingFinished( LoadingStatus ) ),
            this, SLOT( handleLoadingFinished( LoadingStatus ) ) );

    // Register for checkbox changes
    signalMux_.connect( SIGNAL( searchRefreshChanged( int ) ),
            this, SLOT( handleSearchRefreshChanged( int ) ) );
    signalMux_.connect( SIGNAL( ignoreCaseChanged( int ) ),
            this, SLOT( handleIgnoreCaseChanged( int ) ) );

    // Configure the main tabbed widget
    mainTabWidget_.setDocumentMode( true );
    mainTabWidget_.setMovable( true );
    //mainTabWidget_.setTabShape( QTabWidget::Triangular );
    mainTabWidget_.setTabsClosable( true );

    connect( &mainTabWidget_, &TabbedCrawlerWidget::tabCloseRequested ,
            this, QOverload<int>::of(&MainWindow::closeTab) );
    connect( &mainTabWidget_, &TabbedCrawlerWidget::currentChanged,
            this, &MainWindow::currentTabChanged );

    // Establish the QuickFindWidget and mux ( to send requests from the
    // QFWidget to the right window )
    connect( &quickFindWidget_, &QuickFindWidget::patternConfirmed,
             &quickFindMux_, &QuickFindMux::confirmPattern );
    connect( &quickFindWidget_,  &QuickFindWidget::patternUpdated,
             &quickFindMux_, &QuickFindMux::setNewPattern );
    connect( &quickFindWidget_,  &QuickFindWidget::cancelSearch,
             &quickFindMux_, &QuickFindMux::cancelSearch );
    connect( &quickFindWidget_,  &QuickFindWidget::searchForward,
             &quickFindMux_, &QuickFindMux::searchForward );
    connect( &quickFindWidget_,  &QuickFindWidget::searchBackward,
             &quickFindMux_, &QuickFindMux::searchBackward );
    connect( &quickFindWidget_,  &QuickFindWidget::searchNext,
             &quickFindMux_, &QuickFindMux::searchNext );

    // QuickFind changes coming from the views
    connect( &quickFindMux_, &QuickFindMux::patternChanged,
             this, &MainWindow::changeQFPattern );
    connect( &quickFindMux_, &QuickFindMux::notify,
             &quickFindWidget_, &QuickFindWidget::notify );
    connect( &quickFindMux_, &QuickFindMux::clearNotification,
             &quickFindWidget_, &QuickFindWidget::clearNotification );

    // Actions from external instances
    connect( externalCommunicator_.get(), &ExternalCommunicator::loadFile,
             this, &MainWindow::loadFileNonInteractive );

#ifdef GLOGG_SUPPORTS_VERSION_CHECKING
    // Version checker notification
    connect( &versionChecker_, &VersionChecker::newVersionFound,
            this, &MainWindow::newVersionNotification );
#endif

    // Construct the QuickFind bar
    quickFindWidget_.hide();

    QWidget* central_widget = new QWidget();
    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->setContentsMargins( 0, 0, 0, 0 );
    main_layout->addWidget( &mainTabWidget_ );
    main_layout->addWidget( &quickFindWidget_ );
    central_widget->setLayout( main_layout );

    setCentralWidget( central_widget );
}

void MainWindow::reloadGeometry()
{
    QByteArray geometry;

    session_->storedGeometry( &geometry );
    restoreGeometry( geometry );
}

void MainWindow::reloadSession()
{
    int current_file_index = -1;

    for ( const auto& open_file: session_->restore(
               []() { return new CrawlerWidget(); },
               &current_file_index ) )
    {
        QString file_name = { open_file.first };
        CrawlerWidget* crawler_widget = dynamic_cast<CrawlerWidget*>(
                open_file.second );

        assert( crawler_widget );

        mainTabWidget_.addTab( crawler_widget, strippedName( file_name ) );
    }

    if ( current_file_index >= 0 )
        mainTabWidget_.setCurrentIndex( current_file_index );
}

void MainWindow::loadInitialFile( QString fileName )
{
    LOG(logDEBUG) << "loadInitialFile";

    // Is there a file passed as argument?
    if ( !fileName.isEmpty() )
        loadFile( fileName );
}

void MainWindow::startBackgroundTasks()
{
    LOG(logDEBUG) << "startBackgroundTasks";

#ifdef GLOGG_SUPPORTS_VERSION_CHECKING
    versionChecker_.startCheck();
#endif
}

//
// Private functions
//

const MainWindow::EncodingList MainWindow::encoding_list[] = {
    { "&Auto" },
    { "ASCII / &ISO-8859-1" },
    { "&UTF-8" },
    { "CP1251" },
    { "UTF-16LE" },
    { "UTF-16BE" },
    { "UTF-32LE" },
    { "UTF-32BE" },
    { "Local" },
};

// Menu actions
void MainWindow::createActions()
{
    std::shared_ptr<Configuration> config =
        Persistent<Configuration>( "settings" );

    openAction = new QAction(tr("&Open..."), this);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setIcon( QIcon( ":/images/open14.png" ) );
    openAction->setStatusTip(tr("Open a file"));
    connect(openAction, &QAction::triggered, [this](auto){ this->open(); });

    closeAction = new QAction(tr("&Close"), this);
    closeAction->setShortcut(tr("Ctrl+W"));
    closeAction->setStatusTip(tr("Close document"));
    connect(closeAction, &QAction::triggered, [this](auto){ this->closeTab(); });

    closeAllAction = new QAction(tr("Close &All"), this);
    closeAllAction->setStatusTip(tr("Close all documents"));
    connect(closeAllAction, &QAction::triggered, [this](auto){ this->closeAll(); });

    // Recent files
    for (auto i = 0u; i < recentFileActions.size(); ++i) {
        recentFileActions[i] = new QAction(this);
        recentFileActions[i]->setVisible(false);
        connect(recentFileActions[i], &QAction::triggered,
                [this, action = recentFileActions[i]] (auto) {
                    this->openRecentFile( action );
                }
        );
    }

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(tr("Ctrl+Q"));
    exitAction->setStatusTip(tr("Exit the application"));
    connect( exitAction, &QAction::triggered, [this](auto){ this->close(); });

    copyAction = new QAction(tr("&Copy"), this);
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setStatusTip(tr("Copy the selection"));
    connect( copyAction, &QAction::triggered, [this](auto){ this->copy(); });

    selectAllAction = new QAction(tr("Select &All"), this);
    selectAllAction->setShortcut(tr("Ctrl+A"));
    selectAllAction->setStatusTip(tr("Select all the text"));
    connect( selectAllAction, &QAction::triggered,
             [this](auto){ this->selectAll(); });

    findAction = new QAction(tr("&Find..."), this);
    findAction->setShortcut(QKeySequence::Find);
    findAction->setStatusTip(tr("Find the text"));
    connect( findAction, &QAction::triggered,
             [this](auto){ this->find(); });

    overviewVisibleAction = new QAction( tr("Matches &overview"), this );
    overviewVisibleAction->setCheckable( true );
    overviewVisibleAction->setChecked( config->isOverviewVisible() );
    connect( overviewVisibleAction, &QAction::toggled,
            this, &MainWindow::toggleOverviewVisibility );

    lineNumbersVisibleInMainAction =
        new QAction( tr("Line &numbers in main view"), this );
    lineNumbersVisibleInMainAction->setCheckable( true );
    lineNumbersVisibleInMainAction->setChecked( config->mainLineNumbersVisible() );
    connect( lineNumbersVisibleInMainAction,  &QAction::toggled,
             this, &MainWindow::toggleMainLineNumbersVisibility );

    lineNumbersVisibleInFilteredAction =
        new QAction( tr("Line &numbers in filtered view"), this );
    lineNumbersVisibleInFilteredAction->setCheckable( true );
    lineNumbersVisibleInFilteredAction->setChecked( config->filteredLineNumbersVisible() );
    connect( lineNumbersVisibleInFilteredAction, &QAction::toggled,
             this, &MainWindow::toggleFilteredLineNumbersVisibility );

    followAction = new QAction( tr("&Follow File"), this );
    followAction->setShortcut(Qt::Key_F);
    followAction->setCheckable(true);
    connect( followAction, &QAction::toggled,
             this, &MainWindow::followSet );

    reloadAction = new QAction( tr("&Reload"), this );
    reloadAction->setShortcut(QKeySequence::Refresh);
    reloadAction->setIcon( QIcon(":/images/reload14.png") );
    signalMux_.connect( reloadAction, SIGNAL(triggered()), SLOT(reload()) );

    stopAction = new QAction( tr("&Stop"), this );
    stopAction->setIcon( QIcon(":/images/stop14.png") );
    stopAction->setEnabled( true );
    signalMux_.connect( stopAction, SIGNAL(triggered()), SLOT(stopLoading()) );

    filtersAction = new QAction(tr("&Filters..."), this);
    filtersAction->setStatusTip(tr("Show the Filters box"));
    connect( filtersAction, &QAction::triggered,
             [this](auto){ this->filters(); });

    optionsAction = new QAction(tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Show the Options box"));
    connect( optionsAction, &QAction::triggered,
             [this](auto){ this->options(); });

    aboutAction = new QAction(tr("&About"), this);
    aboutAction->setStatusTip(tr("Show the About box"));
    connect( aboutAction, &QAction::triggered,
             [this](auto){ this->about(); });

    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show the Qt library's About box"));
    connect( aboutQtAction, &QAction::triggered,
             [this](auto){ this->aboutQt(); });

    encodingGroup = new QActionGroup( this );

    for ( int i = 0; i < CrawlerWidget::ENCODING_MAX; ++i ) {
        encodingAction[i] = new QAction( tr( encoding_list[i].name ), this );
        encodingAction[i]->setCheckable( true );
        encodingGroup->addAction( encodingAction[i] );
    }

    encodingAction[0]->setStatusTip(tr("Automatically detect the file's encoding"));
    encodingAction[0]->setChecked( true );

    connect( encodingGroup, &QActionGroup::triggered,
            this, &MainWindow::encodingChanged );
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu( tr("&File") );
    fileMenu->addAction( openAction );
    fileMenu->addAction( closeAction );
    fileMenu->addAction( closeAllAction );
    fileMenu->addSeparator();

    assert(recentFileActions.size() == recentFileActionBehaviors.size());
    for (auto i = 0u; i < recentFileActions.size(); ++i) {
        fileMenu->addAction( recentFileActions[i] );
        recentFileActionBehaviors[i] =
            new MenuActionToolTipBehavior(recentFileActions[i], fileMenu, this);
    }

    fileMenu->addSeparator();
    fileMenu->addAction( exitAction );

    editMenu = menuBar()->addMenu( tr("&Edit") );
    editMenu->addAction( copyAction );
    editMenu->addAction( selectAllAction );
    editMenu->addSeparator();
    editMenu->addAction( findAction );

    viewMenu = menuBar()->addMenu( tr("&View") );
    viewMenu->addAction( overviewVisibleAction );
    viewMenu->addSeparator();
    viewMenu->addAction( lineNumbersVisibleInMainAction );
    viewMenu->addAction( lineNumbersVisibleInFilteredAction );
    viewMenu->addSeparator();
    viewMenu->addAction( followAction );
    viewMenu->addSeparator();
    viewMenu->addAction( reloadAction );

    toolsMenu = menuBar()->addMenu( tr("&Tools") );
    toolsMenu->addAction( filtersAction );
    toolsMenu->addSeparator();
    toolsMenu->addAction( optionsAction );

    encodingMenu = menuBar()->addMenu( tr("En&coding") );
    encodingMenu->addAction( encodingAction[0] );
    encodingMenu->addSeparator();
    for ( int i = 1; i < CrawlerWidget::ENCODING_MAX; ++i ) {
        encodingMenu->addAction( encodingAction[i] );
    }

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
    toolBar->setIconSize( QSize( 14, 14 ) );
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
    if ( auto current = currentCrawlerWidget() )
    {
        QString current_file = session_->getFilename( current );
        QFileInfo fileInfo = QFileInfo( current_file );
        defaultDir = fileInfo.path();
    }

    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open file"), defaultDir, tr("All files (*)"));
    if (!fileName.isEmpty())
        loadFile(fileName);
}

// Opens a log file from the recent files list
void MainWindow::openRecentFile(QAction *recentFileAction)
{
    if ( recentFileAction )
        loadFile( recentFileAction->data().toString() );
}

// Close current tab
void MainWindow::closeTab()
{
    int currentIndex = mainTabWidget_.currentIndex();

    if ( currentIndex >= 0 )
    {
        closeTab(currentIndex);
    }
}

// Close all tabs
void MainWindow::closeAll()
{
    while ( mainTabWidget_.count() )
    {
        closeTab(0);
    }
}

// Select all the text in the currently selected view
void MainWindow::selectAll()
{
    CrawlerWidget* current = currentCrawlerWidget();

    if ( current )
        current->selectAll();
}

// Copy the currently selected line into the clipboard
void MainWindow::copy()
{
    static QClipboard* clipboard = QApplication::clipboard();
    CrawlerWidget* current = currentCrawlerWidget();

    if ( current ) {
        clipboard->setText( current->getSelectedText() );

        // Put it in the global selection as well (X11 only)
        clipboard->setText( current->getSelectedText(),
                QClipboard::Selection );
    }
}

// Display the QuickFind bar
void MainWindow::find()
{
    displayQuickFindBar( QuickFindMux::Forward );
}

// Opens the 'Filters' dialog box
void MainWindow::filters()
{
    FiltersDialog dialog(this);
    signalMux_.connect(&dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ));
    dialog.exec();
    signalMux_.disconnect(&dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ));
}

// Opens the 'Options' modal dialog box
void MainWindow::options()
{
    OptionsDialog dialog(this);
    signalMux_.connect(&dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ));
    dialog.exec();
    signalMux_.disconnect(&dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ));
}

// Opens the 'About' dialog box.
void MainWindow::about()
{
    QMessageBox::about(this, tr("About klogg"),
            tr("<h2>klogg " GLOGG_VERSION "</h2>"
                "<p>A fast, advanced log explorer."
#ifdef GLOGG_COMMIT
                "<p>Built " GLOGG_DATE " from " GLOGG_COMMIT
#endif
                "<p><a href=\"https://github.com/variar/klogg\">https://github.com/variar/klogg</a></p>"
                "<p>This is fork of glogg</p>"
                "<p><a href=\"http://glogg.bonnefon.org/\">http://glogg.bonnefon.org/</a></p>"
                "<p>Copyright &copy; 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Nicolas Bonnefon and other contributors"
                "<p>You may modify and redistribute the program under the terms of the GPL (version 3 or later)." ) );
}

// Opens the 'About Qt' dialog box.
void MainWindow::aboutQt()
{
}

void MainWindow::encodingChanged( QAction* action )
{
    int i = 0;
    for ( i = 0; i < CrawlerWidget::ENCODING_MAX; ++i )
        if ( action == encodingAction[i] )
            break;

    LOG(logDEBUG) << "encodingChanged, encoding " << i;
    currentCrawlerWidget()->setEncoding( static_cast<CrawlerWidget::Encoding>( i ) );
    updateInfoLine();
}

void MainWindow::toggleOverviewVisibility( bool isVisible )
{
    std::shared_ptr<Configuration> config =
        Persistent<Configuration>( "settings" );
    config->setOverviewVisible( isVisible );
    emit optionsChanged();
}

void MainWindow::toggleMainLineNumbersVisibility( bool isVisible )
{
    std::shared_ptr<Configuration> config =
        Persistent<Configuration>( "settings" );
    config->setMainLineNumbersVisible( isVisible );
    emit optionsChanged();
}

void MainWindow::toggleFilteredLineNumbersVisibility( bool isVisible )
{
    std::shared_ptr<Configuration> config =
        Persistent<Configuration>( "settings" );
    config->setFilteredLineNumbersVisible( isVisible );
    emit optionsChanged();
}

void MainWindow::changeFollowMode( bool follow )
{
    followAction->setChecked( follow );
}

void MainWindow::lineNumberHandler( LineNumber line )
{
    // The line number received is the internal (starts at 0)
    lineNbField->setText( tr( "Line %1" ).arg( line.get() + 1 ) );
}

void MainWindow::updateLoadingProgress( int progress )
{
    LOG(logDEBUG) << "Loading progress: " << progress;

    QString current_file =
        session_->getFilename( currentCrawlerWidget() );

    // We ignore 0% and 100% to avoid a flash when the file (or update)
    // is very short.
    if ( progress > 0 && progress < 100 ) {
        infoLine->setText( current_file +
                tr( " - Indexing lines... (%1 %)" ).arg( progress ) );
        infoLine->displayGauge( progress );

        stopAction->setEnabled( true );
        reloadAction->setEnabled( false );
    }
}

void MainWindow::handleLoadingFinished( LoadingStatus status )
{
    LOG(logDEBUG) << "handleLoadingFinished success=" <<
        ( status == LoadingStatus::Successful );

    // No file is loading
    loadingFileName.clear();

    if ( status == LoadingStatus::Successful )
    {
        updateInfoLine();

        infoLine->hideGauge();
        stopAction->setEnabled( false );
        reloadAction->setEnabled( true );

        // Now everything is ready, we can finally show the file!
        currentCrawlerWidget()->show();
    }
    else
    {
        if ( status == LoadingStatus::NoMemory )
        {
            QMessageBox alertBox;
            alertBox.setText( "Not enough memory." );
            alertBox.setInformativeText( "The system does not have enough \
memory to hold the index for this file. The file will now be closed." );
            alertBox.setIcon( QMessageBox::Critical );
            alertBox.exec();
        }

        closeTab( mainTabWidget_.currentIndex()  );
    }

    // mainTabWidget_.setEnabled( true );
}

void MainWindow::handleSearchRefreshChanged( int state )
{
    auto config = Persistent<Configuration>( "settings" );
    config->setSearchAutoRefreshDefault( state == Qt::Checked );
}

void MainWindow::handleIgnoreCaseChanged( int state )
{
    auto config = Persistent<Configuration>( "settings" );
    config->setSearchIgnoreCaseDefault( state == Qt::Checked );
}

void MainWindow::closeTab( int index )
{
    auto widget = dynamic_cast<CrawlerWidget*>(
            mainTabWidget_.widget( index ) );

    assert( widget );

    widget->stopLoading();
    mainTabWidget_.removeTab( index );
    session_->close( widget );
    delete widget;
}

void MainWindow::currentTabChanged( int index )
{
    LOG(logDEBUG) << "currentTabChanged";

    if ( index >= 0 )
    {
        CrawlerWidget* crawler_widget = dynamic_cast<CrawlerWidget*>(
                mainTabWidget_.widget( index ) );
        signalMux_.setCurrentDocument( crawler_widget );
        quickFindMux_.registerSelector( crawler_widget );

        // New tab is set up with fonts etc...
        emit optionsChanged();

        // Update the menu bar
        updateMenuBarFromDocument( crawler_widget );

        // Update the title bar
        updateTitleBar( session_->getFilename( crawler_widget ) );
    }
    else
    {
        // No tab left
        signalMux_.setCurrentDocument( nullptr );
        quickFindMux_.registerSelector( nullptr );

        infoLine->hideGauge();
        infoLine->clear();

        updateTitleBar( QString() );
    }
}

void MainWindow::changeQFPattern( const QString& newPattern )
{
    quickFindWidget_.changeDisplayedPattern( newPattern );
}

void MainWindow::loadFileNonInteractive( const QString& file_name )
{
    LOG(logDEBUG) << "loadFileNonInteractive( "
        << file_name.toStdString() << " )";

    loadFile( file_name );

    // Try to get the window to the front
    // This is a bit of a hack but has been tested on:
    // Qt 5.3 / Gnome / Linux
    // Qt 4.8 / Win7
#ifdef _WIN32
    // Hack copied from http://qt-project.org/forums/viewthread/6164
    ::SetWindowPos((HWND) effectiveWinId(), HWND_TOPMOST,
            0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    ::SetWindowPos((HWND) effectiveWinId(), HWND_NOTOPMOST,
            0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#else
    Qt::WindowFlags window_flags = windowFlags();
    window_flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags( window_flags );
#endif

    activateWindow();
    raise();

#ifndef _WIN32
    window_flags = windowFlags();
    window_flags &= ~Qt::WindowStaysOnTopHint;
    setWindowFlags( window_flags );
    showNormal();
#endif

    currentCrawlerWidget()->setFocus();
}

void MainWindow::newVersionNotification( const QString& new_version )
{
    LOG(logDEBUG) << "newVersionNotification( " <<
        new_version.toStdString() << " )";

    QMessageBox msgBox;
    msgBox.setText( QString( "A new version of glogg (%1) is available for download <p>"
                "<a href=\"http://glogg.bonnefon.org/download.html\">http://glogg.bonnefon.org/download.html</a>" 
                ).arg( new_version ) );
    msgBox.exec();
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

void MainWindow::keyPressEvent( QKeyEvent* keyEvent )
{
    LOG(logDEBUG4) << "keyPressEvent received";

    switch ( keyEvent->text().at(0).toLatin1() ) {
        case '/':
            displayQuickFindBar( QuickFindMux::Forward );
            break;
        case '?':
            displayQuickFindBar( QuickFindMux::Backward );
            break;
        default:
            keyEvent->ignore();
    }

    if ( !keyEvent->isAccepted() )
        QMainWindow::keyPressEvent( keyEvent );
}

//
// Private functions
//

// Create a CrawlerWidget for the passed file, start its loading
// and update the title bar.
// The loading is done asynchronously.
bool MainWindow::loadFile( const QString& fileName )
{
    LOG(logDEBUG) << "loadFile ( " << fileName.toStdString() << " )";

    // First check if the file is already open...
    CrawlerWidget* existing_crawler = dynamic_cast<CrawlerWidget*>(
            session_->getViewIfOpen( fileName ) );
    if ( existing_crawler ) {
        // ... and switch to it.
        mainTabWidget_.setCurrentWidget( existing_crawler );

        return true;
    }

    // Load the file
    loadingFileName = fileName;

    try {
        CrawlerWidget* crawler_widget = dynamic_cast<CrawlerWidget*>(
                session_->open( fileName,
                    []() { return new CrawlerWidget(); } ) );
        assert( crawler_widget );

        // We won't show the widget until the file is fully loaded
        crawler_widget->hide();

        // We disable the tab widget to avoid having someone switch
        // tab during loading. (maybe FIXME)
        //mainTabWidget_.setEnabled( false );

        int index = mainTabWidget_.addTab(
                crawler_widget, strippedName( fileName ) );

        // Setting the new tab, the user will see a blank page for the duration
        // of the loading, with no way to switch to another tab
        mainTabWidget_.setCurrentIndex( index );

        // Update the recent files list
        // (reload the list first in case another glogg changed it)
        GetPersistentInfo().retrieve( "recentFiles" );
        recentFiles_->addRecent( fileName );
        GetPersistentInfo().save( "recentFiles" );
        updateRecentFileActions();
    }
    catch ( FileUnreadableErr ) {
        LOG(logDEBUG) << "Can't open file " << fileName.toStdString();
        return false;
    }

    LOG(logDEBUG) << "Success loading file " << fileName.toStdString();
    return true;

}

// Strips the passed filename from its directory part.
QString MainWindow::strippedName( const QString& fullFileName ) const
{
    return QFileInfo( fullFileName ).fileName();
}

// Return the currently active CrawlerWidget, or NULL if none
CrawlerWidget* MainWindow::currentCrawlerWidget() const
{
    auto current = dynamic_cast<CrawlerWidget*>(
            mainTabWidget_.currentWidget() );

    return current;
}

// Update the title bar.
void MainWindow::updateTitleBar( const QString& file_name )
{
    QString shownName = tr( "Untitled" );
    if ( !file_name.isEmpty() )
        shownName = strippedName( file_name );

    setWindowTitle(
            tr("%1 - %2").arg(shownName, tr("klogg"))
#ifdef GLOGG_COMMIT
            + " (build " GLOGG_VERSION ")"
#endif
            );
}

// Updates the actions for the recent files.
// Must be called after having added a new name to the list.
void MainWindow::updateRecentFileActions()
{
    QStringList recent_files = recentFiles_->recentFiles();

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

// Update our menu bar to match the settings of the crawler
// (used when the tab is changed)
void MainWindow::updateMenuBarFromDocument( const CrawlerWidget* crawler )
{
    auto encoding = crawler->encodingSetting();
    encodingAction[static_cast<int>( encoding )]->setChecked( true );
    bool follow = crawler->isFollowEnabled();
    followAction->setChecked( follow );
}

// Update the top info line from the session
void MainWindow::updateInfoLine()
{
    QLocale defaultLocale;

    // Following should always work as we will only receive enter
    // this slot if there is a crawler connected.
    QString current_file =
        session_->getFilename( currentCrawlerWidget() );

    uint64_t fileSize;
    uint32_t fileNbLine;
    QDateTime lastModified;

    session_->getFileInfo( currentCrawlerWidget(),
            &fileSize, &fileNbLine, &lastModified );
    if ( lastModified.isValid() ) {
        const QString date =
            defaultLocale.toString( lastModified, QLocale::NarrowFormat );
        infoLine->setText( tr( "%1 (%2 - %3 lines - modified on %4 - %5)" )
                .arg(current_file, readableSize(fileSize))
                .arg(fileNbLine)
                .arg(date, currentCrawlerWidget()->encodingText()) );
    }
    else {
        infoLine->setText( tr( "%1 (%2 - %3 lines - %4)" )
                .arg(current_file, readableSize(fileSize))
                .arg(fileNbLine)
                .arg(currentCrawlerWidget()->encodingText()) );
    }
}

// Write settings to permanent storage
void MainWindow::writeSettings()
{
    // Save the session
    // Generate the ordered list of widgets and their topLine
    std::vector<
            std::tuple<const ViewInterface*, uint64_t, std::shared_ptr<const ViewContextInterface>>
        > widget_list;
    for ( int i = 0; i < mainTabWidget_.count(); ++i )
    {
        auto view = dynamic_cast<const ViewInterface*>( mainTabWidget_.widget( i ) );
        widget_list.emplace_back(
                view,
                0UL,
                view->context() );
    }
    session_->save( widget_list, saveGeometry() );

    // User settings
    GetPersistentInfo().save( QString( "settings" ) );
}

// Read settings from permanent storage
void MainWindow::readSettings()
{
    // Get and restore the session
    // GetPersistentInfo().retrieve( QString( "session" ) );
    // SessionInfo session = Persistent<SessionInfo>( "session" );
    /*
     * FIXME: should be in the session
    crawlerWidget->restoreState( session.crawlerState() );
    */

    // History of recent files
    GetPersistentInfo().retrieve( QString( "recentFiles" ) );
    updateRecentFileActions();

    // GetPersistentInfo().retrieve( QString( "settings" ) );
    GetPersistentInfo().retrieve( QString( "filterSet" ) );
}

void MainWindow::displayQuickFindBar( QuickFindMux::QFDirection direction )
{
    LOG(logDEBUG) << "MainWindow::displayQuickFindBar";

    // Warn crawlers so they can save the position of the focus in order
    // to do incremental search in the right view.
    emit enteringQuickFind();

    quickFindMux_.setDirection( direction );
    quickFindWidget_.userActivate();
}

// Returns the size in human readable format
static QString readableSize( qint64 size )
{
    static const QString sizeStrs[] = {
        QObject::tr("B"), QObject::tr("KiB"), QObject::tr("MiB"),
        QObject::tr("GiB"), QObject::tr("TiB") };

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
