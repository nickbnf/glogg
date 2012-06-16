/*
 * Copyright (C) 2009, 2010, 2011, 2012 Nicolas Bonnefon and other contributors
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

// This file implements the CrawlerWidget class.
// It is responsible for creating and managing the two views and all
// the UI elements.  It implements the connection between the UI elements.
// It also owns the sets of data (full and filtered).

#include "log.h"

#include <Qt>
#include <QApplication>
#include <QFile>
#include <QLineEdit>
#include <QFileInfo>
#include <QKeyEvent>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QListView>

#include "crawlerwidget.h"

#include "quickfindpattern.h"
#include "overview.h"
#include "infoline.h"
#include "savedsearches.h"
#include "quickfindwidget.h"
#include "persistentinfo.h"
#include "configuration.h"

// Palette for error signaling (yellow background)
const QPalette CrawlerWidget::errorPalette( QColor( "yellow" ) );

// Constructor makes all the child widgets and set up connections.
CrawlerWidget::CrawlerWidget(SavedSearches* searches, QWidget *parent)
        : QSplitter(parent)
{
    setOrientation(Qt::Vertical);

    // Initialise internal data (with empty file and search)
    logData_          = new LogData();
    logFilteredData_  = logData_->getNewFilteredData();

    // Initialise the QFP object (one for both views)
    // This is the confirmed pattern used by n/N and coloured in yellow.
    quickFindPattern_ = new QuickFindPattern();

    // The matches overview
    overview_ = new Overview();

    // The views
    logMainView  = new LogMainView( logData_, quickFindPattern_, overview_ );
    bottomWindow = new QWidget;
    filteredView = new FilteredView( logFilteredData_, quickFindPattern_ );

    savedSearches = searches;

    // Construct the visibility button
    visibilityModel_ = new QStandardItemModel( this );

    QStandardItem *marksAndMatchesItem = new QStandardItem( tr( "Marks and matches" ) );
    QPixmap marksAndMatchesPixmap( 16, 10 );
    marksAndMatchesPixmap.fill( Qt::gray );
    marksAndMatchesItem->setIcon( QIcon( marksAndMatchesPixmap ) );
    marksAndMatchesItem->setData( FilteredView::MarksAndMatches );
    visibilityModel_->appendRow( marksAndMatchesItem );

    QStandardItem *marksItem = new QStandardItem( tr( "Marks" ) );
    QPixmap marksPixmap( 16, 10 );
    marksPixmap.fill( Qt::blue );
    marksItem->setIcon( QIcon( marksPixmap ) );
    marksItem->setData( FilteredView::MarksOnly );
    visibilityModel_->appendRow( marksItem );

    QStandardItem *matchesItem = new QStandardItem( tr( "Matches" ) );
    QPixmap matchesPixmap( 16, 10 );
    matchesPixmap.fill( Qt::red );
    matchesItem->setIcon( QIcon( matchesPixmap ) );
    matchesItem->setData( FilteredView::MatchesOnly );
    visibilityModel_->appendRow( matchesItem );

    QListView *visibilityView = new QListView( this );
    visibilityView->setMovement( QListView::Static );
    visibilityView->setMinimumWidth( 170 ); // Only needed with custom style-sheet

    visibilityBox = new QComboBox();
    visibilityBox->setModel( visibilityModel_ );
    visibilityBox->setView( visibilityView );

    // Select "Marks and matches" by default (same default as the filtered view)
    visibilityBox->setCurrentIndex( 0 );

    // TODO: Maybe there is some way to set the popup width to be
    // sized-to-content (as it is when the stylesheet is not overriden) in the
    // stylesheet as opposed to setting a hard min-width on the view above.
    visibilityBox->setStyleSheet( " \
        QComboBox:on {\
            padding: 1px 2px 1px 6px;\
            width: 19px;\
        } \
        QComboBox:!on {\
            padding: 1px 2px 1px 7px;\
            width: 19px;\
            height: 16px;\
            border: 1px solid gray;\
        } \
        QComboBox::drop-down::down-arrow {\
            width: 0px;\
            border-width: 0px;\
        } \
" );

    // Construct the Search Info line
    searchInfoLine = new InfoLine();
    searchInfoLine->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
    searchInfoLine->setLineWidth( 1 );
    searchInfoLineDefaultPalette = searchInfoLine->palette();

    ignoreCaseCheck = new QCheckBox( "Ignore &case" );
    searchRefreshCheck = new QCheckBox( "Auto-&refresh" );

    // Construct the Search line
    searchLabel = new QLabel(tr("&Text: "));
    searchLineEdit = new QComboBox;
    searchLineEdit->setEditable( true );
    searchLineEdit->setCompleter( 0 );
    searchLineEdit->addItems( savedSearches->recentSearches() );
    searchLineEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    searchLineEdit->setSizeAdjustPolicy( QComboBox::AdjustToMinimumContentsLengthWithIcon );

    searchLabel->setBuddy( searchLineEdit );

    searchButton = new QToolButton();
    searchButton->setText( tr("&Search") );
    searchButton->setAutoRaise( true );

    stopButton = new QToolButton();
    stopButton->setIcon( QIcon(":/images/stop16.png") );
    stopButton->setAutoRaise( true );
    stopButton->setEnabled( false );

    // Construct the QuickFind bar
    quickFindWidget_ = new QuickFindWidget();

    QHBoxLayout* searchLineLayout = new QHBoxLayout;
    searchLineLayout->addWidget(searchLabel);
    searchLineLayout->addWidget(searchLineEdit);
    searchLineLayout->addWidget(searchButton);
    searchLineLayout->addWidget(stopButton);
    searchLineLayout->setContentsMargins(6, 0, 6, 0);
    stopButton->setSizePolicy( QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum ) );
    searchButton->setSizePolicy( QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum ) );

    QHBoxLayout* searchInfoLineLayout = new QHBoxLayout;
    searchInfoLineLayout->addWidget( visibilityBox );
    searchInfoLineLayout->addWidget( searchInfoLine );
    searchInfoLineLayout->addWidget( ignoreCaseCheck );
    searchInfoLineLayout->addWidget( searchRefreshCheck );

    // Construct the bottom window
    quickFindWidget_->hide();
    QVBoxLayout* bottomMainLayout = new QVBoxLayout;
    bottomMainLayout->addLayout(searchLineLayout);
    bottomMainLayout->addLayout(searchInfoLineLayout);
    bottomMainLayout->addWidget(filteredView);
    bottomMainLayout->addWidget(quickFindWidget_);
    bottomMainLayout->setContentsMargins(2, 1, 2, 1);
    bottomWindow->setLayout(bottomMainLayout);

    addWidget(logMainView);
    addWidget(bottomWindow);

    // Default splitter position (usually overridden by the config file)
    QList<int> splitterSizes;
    splitterSizes += 400;
    splitterSizes += 100;
    setSizes( splitterSizes );

    // Connect the signals
    connect(searchLineEdit->lineEdit(), SIGNAL( returnPressed() ),
            searchButton, SIGNAL( clicked() ));
    connect(searchLineEdit->lineEdit(), SIGNAL( textEdited( const QString& ) ),
            this, SLOT( searchTextChangeHandler() ));
    connect(searchButton, SIGNAL( clicked() ),
            this, SLOT( startNewSearch() ) );
    connect(stopButton, SIGNAL( clicked() ),
            this, SLOT( stopSearch() ) );

    connect(visibilityBox, SIGNAL( currentIndexChanged( int ) ),
            this, SLOT( changeFilteredViewVisibility( int ) ) );

    connect(logMainView, SIGNAL( newSelection( int ) ),
            logMainView, SLOT( update() ) );
    connect(filteredView, SIGNAL( newSelection( int ) ),
            this, SLOT( jumpToMatchingLine( int ) ) );
    connect(filteredView, SIGNAL( newSelection( int ) ),
            filteredView, SLOT( update() ) );
    connect(logMainView, SIGNAL( updateLineNumber( int ) ),
            this, SIGNAL( updateLineNumber( int ) ) );
    connect(logMainView, SIGNAL( markLine( qint64 ) ),
            this, SLOT( markLineFromMain( qint64 ) ) );
    connect(filteredView, SIGNAL( markLine( qint64 ) ),
            this, SLOT( markLineFromFiltered( qint64 ) ) );

    connect(logMainView, SIGNAL( addToSearch( const QString& ) ),
            this, SLOT( addToSearch( const QString& ) ) );
    connect(filteredView, SIGNAL( addToSearch( const QString& ) ),
            this, SLOT( addToSearch( const QString& ) ) );

    // Follow option (up and down)
    connect(this, SIGNAL( followSet( bool ) ),
            logMainView, SLOT( followSet( bool ) ) );
    connect(logMainView, SIGNAL( followDisabled() ),
            this, SIGNAL( followDisabled() ) );

    connect( logFilteredData_, SIGNAL( searchProgressed( int, int ) ),
            this, SLOT( updateFilteredView( int, int ) ) );

    // QuickFind
    connect( quickFindWidget_, SIGNAL( patternConfirmed( const QString& ) ),
            this, SLOT( applyNewQFPattern( const QString& ) ) );
    connect( quickFindWidget_, SIGNAL( close() ),
            this, SLOT( hideQuickFindBar() ) );
    connect( quickFindWidget_, SIGNAL( searchForward() ),
            this, SLOT( searchForward() ) );
    connect( quickFindWidget_, SIGNAL( searchBackward() ),
            this, SLOT( searchBackward() ) );

    // QuickFind changes coming from the views
    connect(logMainView, SIGNAL( changeQuickFind( const QString& ) ),
            this, SLOT( changeQFPattern( const QString& ) ) );
    connect(filteredView, SIGNAL( changeQuickFind( const QString& ) ),
            this, SLOT( changeQFPattern( const QString& ) ) );
    connect(logMainView, SIGNAL( notifyQuickFind( const QString& ) ),
            quickFindWidget_, SLOT( notify( const QString& ) ) );
    connect(filteredView, SIGNAL( notifyQuickFind( const QString& ) ),
            quickFindWidget_, SLOT( notify( const QString& ) ) );
    connect(logMainView, SIGNAL( clearQuickFindNotification() ),
            quickFindWidget_, SLOT( clearNotification() ) );
    connect(filteredView, SIGNAL( clearQuickFindNotification() ),
            quickFindWidget_, SLOT( clearNotification() ) );

    // Sent load file update to MainWindow (for status update)
    connect( logData_, SIGNAL( loadingProgressed( int ) ),
            this, SIGNAL( loadingProgressed( int ) ) );
    connect( logData_, SIGNAL( loadingFinished( bool ) ),
            this, SLOT( loadingFinishedHandler( bool ) ) );
    connect( logData_, SIGNAL( fileChanged( LogData::MonitoredFileStatus ) ),
            this, SLOT( fileChangedHandler( LogData::MonitoredFileStatus ) ) );

    // Search auto-refresh
    connect( searchRefreshCheck, SIGNAL( stateChanged( int ) ),
            this, SLOT( searchRefreshChangedHandler( int ) ) );
}

// Start the asynchronous loading of a file.
bool CrawlerWidget::readFile( const QString& fileName, int )
{
    QFileInfo fileInfo( fileName );
    if ( fileInfo.isReadable() )
    {
        LOG(logDEBUG) << "Entering readFile " << fileName.toStdString();

        // First we cancel any in progress search and loading
        stopLoading();

        // The file exist, so we invalidate the search, remove all marks
        // and redraw the screen.
        replaceCurrentSearch( "" );
        logFilteredData_->clearMarks();
        logData_->attachFile( fileName );
        logMainView->updateData();

        // Forbid starting a search when loading in progress
        // searchButton->setEnabled( false );

        return true;
    }
    else {
        return false;
    }
}

void CrawlerWidget::stopLoading()
{
    logFilteredData_->interruptSearch();
    logData_->interruptLoading();
}

void CrawlerWidget::getFileInfo( qint64* fileSize, int* fileNbLine,
       QDateTime* lastModified ) const
{
    *fileSize = logData_->getFileSize();
    *fileNbLine = logData_->getNbLine();
    *lastModified = logData_->getLastModifiedDate();
}

// The top line is first one on the main display
int CrawlerWidget::getTopLine() const
{
    return logMainView->getTopLine();
}

QString CrawlerWidget::getSelectedText() const
{
    if ( filteredView->hasFocus() )
        return filteredView->getSelection();
    else
        return logMainView->getSelection();
}

//
// Events handlers
//

void CrawlerWidget::keyPressEvent( QKeyEvent* keyEvent )
{
    LOG(logDEBUG4) << "keyPressEvent received";

    switch ( (keyEvent->text())[0].toAscii() ) {
        case '/':
        case '?':
            displayQuickFindBar();
            break;
        default:
            keyEvent->ignore();
    }

    if ( !keyEvent->isAccepted() )
        QSplitter::keyPressEvent( keyEvent );
}

//
// Slots
//

void CrawlerWidget::startNewSearch()
{
    // Record the search line in the recent list
    // (reload the list first in case another glogg changed it)
    GetPersistentInfo().retrieve( "savedSearches" );
    savedSearches->addRecent( searchLineEdit->currentText() );
    GetPersistentInfo().save( "savedSearches" );

    // Update the SearchLine (history)
    updateSearchCombo();
    // Call the private function to do the search
    replaceCurrentSearch( searchLineEdit->currentText() );
}

void CrawlerWidget::stopSearch()
{
    logFilteredData_->interruptSearch();
    searchState_.stopSearch();
    printSearchInfoMessage();
}

// When receiving the 'newDataAvailable' signal from LogFilteredData
void CrawlerWidget::updateFilteredView( int nbMatches, int progress )
{
    LOG(logDEBUG) << "updateFilteredView received.";

    if ( progress == 100 ) {
        // Searching done
        printSearchInfoMessage( nbMatches );
        searchInfoLine->hideGauge();
        // De-activate the stop button
        stopButton->setEnabled( false );
    }
    else {
        // Search in progress
        // We ignore 0% and 100% to avoid a flash when the search is very short
        if ( progress > 0 ) {
            searchInfoLine->setText(
                    tr("Search in progress (%1 %)... %2 match%3 found so far.")
                    .arg( progress )
                    .arg( nbMatches )
                    .arg( nbMatches > 1 ? "es" : "" ) );
            searchInfoLine->displayGauge( progress );
        }
    }

    // Recompute the content of the filtered window.
    filteredView->updateData();

    // Update the match overview
    overview_->updateData( logData_->getNbLine() );

    // Also update the top window for the coloured bullets.
    update();
}

void CrawlerWidget::jumpToMatchingLine(int filteredLineNb)
{
    int mainViewLine = logFilteredData_->getMatchingLineNumber(filteredLineNb);
    logMainView->selectAndDisplayLine(mainViewLine);  // FIXME: should be done with a signal.
}

void CrawlerWidget::markLineFromMain( qint64 line )
{
    if ( logFilteredData_->isLineMarked( line ) )
        logFilteredData_->deleteMark( line );
    else
        logFilteredData_->addMark( line );

    // Recompute the content of the filtered window.
    filteredView->updateData();

    // Update the match overview
    overview_->updateData( logData_->getNbLine() );

    // Also update the top window for the coloured bullets.
    update();
}

void CrawlerWidget::markLineFromFiltered( qint64 line )
{
    qint64 line_in_file = logFilteredData_->getMatchingLineNumber( line );
    if ( logFilteredData_->filteredLineTypeByIndex( line )
            == LogFilteredData::Mark )
        logFilteredData_->deleteMark( line_in_file );
    else
        logFilteredData_->addMark( line_in_file );

    // Recompute the content of the filtered window.
    filteredView->updateData();

    // Update the match overview
    overview_->updateData( logData_->getNbLine() );

    // Also update the top window for the coloured bullets.
    update();
}

void CrawlerWidget::applyConfiguration()
{
    Configuration& config = Persistent<Configuration>( "settings" );
    QFont font = config.mainFont();

    LOG(logDEBUG) << "CrawlerWidget::applyConfiguration";

    logMainView->setFont(font);
    filteredView->setFont(font);

    logMainView->setLineNumbersVisible( config.lineNumbersVisible() );
    filteredView->setLineNumbersVisible( config.lineNumbersVisible() );

    overview_->setVisible( config.isOverviewVisible() );
    logMainView->refreshOverview();

    logMainView->updateDisplaySize();
    logMainView->update();
    filteredView->updateDisplaySize();
    filteredView->update();

    // Update the SearchLine (history)
    updateSearchCombo();
}

void CrawlerWidget::loadingFinishedHandler( bool success )
{
    // We need to refresh the main window because the view lines on the
    // overview have probably changed.
    overview_->updateData( logData_->getNbLine() );

    // FIXME, handle topLine
    // logMainView->updateData( logData_, topLine );
    logMainView->updateData();

    // searchButton->setEnabled( true );

    // See if we need to auto-refresh the search
    if ( searchState_.isAutorefreshAllowed() ) {
        LOG(logDEBUG) << "Refreshing the search";
        logFilteredData_->updateSearch();
    }

    emit loadingFinished( success );
}

void CrawlerWidget::fileChangedHandler( LogData::MonitoredFileStatus status )
{
    // Handle the case where the file has been truncated
    if ( status == LogData::Truncated ) {
        // Clear all marks (TODO offer the option to keep them)
        logFilteredData_->clearMarks();
        if ( ! searchInfoLine->text().isEmpty() ) {
            // Invalidate the search
            logFilteredData_->clearSearch();
            filteredView->updateData();
            searchState_.truncateFile();
            printSearchInfoMessage();
        }
    }
}

void CrawlerWidget::displayQuickFindBar()
{
    LOG(logDEBUG) << "CrawlerWidget::displayQuickFindBar";

    // Remember who had the focus
    qfSavedFocus_ = QApplication::focusWidget();

    quickFindWidget_->userActivate();
}

void CrawlerWidget::hideQuickFindBar()
{
    // Restore the focus once the QFBar has been hidden
    qfSavedFocus_->setFocus();
}

void CrawlerWidget::applyNewQFPattern( const QString& newPattern )
{
    quickFindPattern_->changeSearchPattern( newPattern );
}

void CrawlerWidget::changeQFPattern( const QString& newPattern )
{
    quickFindWidget_->changeDisplayedPattern( newPattern );
    quickFindPattern_->changeSearchPattern( newPattern );
}

// Returns a pointer to the window in which the search should be done
AbstractLogView* CrawlerWidget::searchableWidget() const
{
    QWidget* searchableWidget;

    // Search in the window that has focus, or the window where 'Find' was
    // called from, or the main window.
    if ( filteredView->hasFocus() || logMainView->hasFocus() )
        searchableWidget = QApplication::focusWidget();
    else
        searchableWidget = qfSavedFocus_;

    if ( AbstractLogView* view = qobject_cast<AbstractLogView*>( searchableWidget ) )
        return view;
    else
        return logMainView;
}

void CrawlerWidget::searchForward()
{
    LOG(logDEBUG) << "CrawlerWidget::searchForward";

    searchableWidget()->searchForward();
}

void CrawlerWidget::searchBackward()
{
    LOG(logDEBUG) << "CrawlerWidget::searchBackward";

    searchableWidget()->searchBackward();
}

void CrawlerWidget::searchRefreshChangedHandler( int state )
{
    searchState_.setAutorefresh( state == Qt::Checked );
    printSearchInfoMessage( logFilteredData_->getNbMatches() );
}

void CrawlerWidget::searchTextChangeHandler()
{
    // We suspend auto-refresh
    searchState_.changeExpression();
    printSearchInfoMessage( logFilteredData_->getNbMatches() );
}

void CrawlerWidget::changeFilteredViewVisibility( int index )
{
    QStandardItem* item = visibilityModel_->item( index );
    FilteredView::Visibility visibility =
        static_cast< FilteredView::Visibility>( item->data().toInt() );

    filteredView->setVisibility( visibility );
}

void CrawlerWidget::addToSearch( const QString& string )
{
    QString text = searchLineEdit->currentText();

    if ( text.isEmpty() )
        text = string;
    else {
        // Escape the regexp chars from the string before adding it.
        text += ( '|' + QRegExp::escape( string ) );
    }

    searchLineEdit->setEditText( text );

    // Set the focus to lineEdit so that the user can press 'Return' immediately
    searchLineEdit->lineEdit()->setFocus();
}

//
// Private functions
//

// Create a new search using the text passed, replace the currently
// used one and destroy the old one.
void CrawlerWidget::replaceCurrentSearch( const QString& searchText )
{
    // Interrupt the search if it's ongoing
    logFilteredData_->interruptSearch();

    // We have to wait for the last search update (100%)
    // before clearing/restarting to avoid having remaining results.

    // FIXME: this is a bit of a hack, we call processEvents
    // for Qt to empty its event queue, including (hopefully)
    // the search update event sent by logFilteredData_. It saves
    // us the overhead of having proper sync.
    QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );

    if ( !searchText.isEmpty() ) {
        // Determine the type of regexp depending on the config
        QRegExp::PatternSyntax syntax;
        static Configuration& config = Persistent<Configuration>( "settings" );
        switch ( config.mainRegexpType() ) {
            case Wildcard:
                syntax = QRegExp::Wildcard;
                break;
            case FixedString:
                syntax = QRegExp::FixedString;
                break;
            default:
                syntax = QRegExp::RegExp2;
                break;
        }

        // Set the pattern case insensitive if needed
        Qt::CaseSensitivity case_sensitivity = Qt::CaseSensitive;
        if ( ignoreCaseCheck->checkState() == Qt::Checked )
            case_sensitivity = Qt::CaseInsensitive;

        // Constructs the regexp
        QRegExp regexp( searchText, case_sensitivity, syntax );

        if ( regexp.isValid() ) {
            // Activate the stop button
            stopButton->setEnabled( true );
            // Start a new asynchronous search
            logFilteredData_->runSearch( regexp );
            // Accept auto-refresh of the search
            searchState_.startSearch();
        }
        else {
            // The regexp is wrong
            logFilteredData_->clearSearch();
            filteredView->updateData();
            searchState_.resetState();

            // Inform the user
            QString errorMessage = tr("Error in expression: ");
            errorMessage += regexp.errorString();
            searchInfoLine->setPalette( errorPalette );
            searchInfoLine->setText( errorMessage );
        }
    }
    else {
        logFilteredData_->clearSearch();
        filteredView->updateData();
        searchState_.resetState();
        printSearchInfoMessage();
    }
    // Connect the search to the top view
    logMainView->useNewFiltering( logFilteredData_ );
}

// Updates the content of the drop down list for the saved searches,
// called when the SavedSearch has been changed.
void CrawlerWidget::updateSearchCombo()
{
    const QString text = searchLineEdit->lineEdit()->text();
    searchLineEdit->clear();
    searchLineEdit->addItems( savedSearches->recentSearches() );
    // In case we had something that wasn't added to the list (blank...):
    searchLineEdit->lineEdit()->setText( text );
}

// Print the search info message.
void CrawlerWidget::printSearchInfoMessage( int nbMatches )
{
    QString text;

    switch ( searchState_.getState() ) {
        case SearchState::NoSearch:
            // Blank text is fine
            break;
        case SearchState::Static:
            text = tr("%1 match%2 found.").arg( nbMatches )
                .arg( nbMatches > 1 ? "es" : "" );
            break;
        case SearchState::Autorefreshing:
            text = tr("%1 match%2 found. Search is auto-refreshing...").arg( nbMatches )
                .arg( nbMatches > 1 ? "es" : "" );
            break;
        case SearchState::FileTruncated:
            text = tr("File truncated on disk, previous search results are not valid anymore.");
            break;
    }

    searchInfoLine->setPalette( searchInfoLineDefaultPalette );
    searchInfoLine->setText( text );
}

//
// SearchState implementation
//
void CrawlerWidget::SearchState::resetState()
{
    state_ = NoSearch;
}

void CrawlerWidget::SearchState::setAutorefresh( bool refresh )
{
    autoRefreshRequested_ = refresh;

    if ( refresh ) {
        if ( state_ == Static )
            state_ = Autorefreshing;
    }
    else {
        if ( state_ == Autorefreshing )
            state_ = Static;
    }
}

void CrawlerWidget::SearchState::truncateFile()
{
    state_ = FileTruncated;
}

void CrawlerWidget::SearchState::changeExpression()
{
    if ( state_ == Autorefreshing )
        state_ = Static;
}

void CrawlerWidget::SearchState::stopSearch()
{
    if ( state_ == Autorefreshing )
        state_ = Static;
}

void CrawlerWidget::SearchState::startSearch()
{
    if ( autoRefreshRequested_ )
        state_ = Autorefreshing;
    else
        state_ = Static;
}
