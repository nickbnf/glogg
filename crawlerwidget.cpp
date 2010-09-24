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

#include "crawlerwidget.h"

#include "configuration.h"

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

    // The views
    logMainView  = new LogMainView( logData_, quickFindPattern_ );
    bottomWindow = new QWidget;
    filteredView = new FilteredView( logFilteredData_, quickFindPattern_ );

    savedSearches = searches;

    // Construct the Search Info line
    searchInfoLine = new InfoLine();
    searchInfoLine->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
    searchInfoLine->setLineWidth( 1 );
    stopButton = new QToolButton();
    stopButton->setIcon( QIcon(":/images/stop16.png") );
    stopButton->setAutoRaise( true );
    stopButton->setEnabled( false );

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

    // Construct the QuickSearch bar
    quickFindWidget_ = new QuickFindWidget();

    QHBoxLayout* searchLineLayout = new QHBoxLayout;
    searchLineLayout->addWidget(searchLabel);
    searchLineLayout->addWidget(searchLineEdit);
    searchLineLayout->addWidget(searchButton);
    searchLineLayout->addWidget(stopButton);
    searchLineLayout->setContentsMargins(6, 0, 6, 0);
    stopButton->setSizePolicy( QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum ) );
    searchButton->setSizePolicy( QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum ) );

    // Construct the bottom window
    quickFindWidget_->hide();
    QVBoxLayout* bottomMainLayout = new QVBoxLayout;
    bottomMainLayout->addLayout(searchLineLayout);
    bottomMainLayout->addWidget(searchInfoLine);
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
    connect(searchButton, SIGNAL( clicked() ),
            this, SLOT( startNewSearch() ) );
    connect(stopButton, SIGNAL( clicked() ),
            this, SLOT( stopSearch() ) );

    connect(logMainView, SIGNAL( newSelection( int ) ),
            logMainView, SLOT( update() ) );
    connect(filteredView, SIGNAL( newSelection( int ) ),
            this, SLOT( jumpToMatchingLine( int ) ) );
    connect(filteredView, SIGNAL( newSelection( int ) ),
            filteredView, SLOT( update() ) );
    connect(logMainView, SIGNAL( updateLineNumber( int ) ),
            this, SIGNAL( updateLineNumber( int ) ) );

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
    connect(filteredView, SIGNAL( changeQuickFind( QString& ) ),
            this, SLOT( changeQFPattern( const QString& ) ) );

    // Sent load file update to MainWindow (for status update)
    connect( logData_, SIGNAL( loadingProgressed( int ) ),
            this, SIGNAL( loadingProgressed( int ) ) );
    connect( logData_, SIGNAL( loadingFinished( bool ) ),
            this, SLOT( loadingFinishedHandler( bool ) ) );
    connect( logData_, SIGNAL( fileChanged( LogData::MonitoredFileStatus ) ),
            this, SLOT( fileChangedHandler( LogData::MonitoredFileStatus ) ) );
}

// Start the asynchronous loading of a file.
bool CrawlerWidget::readFile( const QString& fileName, int topLine )
{
    QFileInfo fileInfo( fileName );
    if ( fileInfo.isReadable() )
    {
        LOG(logDEBUG) << "Entering readFile " << fileName.toStdString();

        // First we cancel any in progress search and loading
        stopLoading();

        // The file exist, so we invalidate the search
        // and redraw the screen.
        logData_->attachFile( fileName );
        replaceCurrentSearch( "" );
        logMainView->updateData();

        // Forbid starting a search when loading in progress
        searchButton->setEnabled( false );

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
    savedSearches->addRecent( searchLineEdit->currentText() );

    // Update the SearchLine (history)
    updateSearchCombo();
    // Call the private function to do the search
    replaceCurrentSearch( searchLineEdit->currentText() );
}

void CrawlerWidget::stopSearch()
{
    logFilteredData_->interruptSearch();
}

// When receiving the 'newDataAvailable' signal from LogFilteredData
void CrawlerWidget::updateFilteredView( int nbMatches, int progress )
{
    LOG(logDEBUG) << "updateFilteredView received.";

    if ( progress == 100 ) {
        // Searching done
        searchInfoLine->setText(
                tr("%1 match%2 found.").arg( nbMatches )
                .arg( nbMatches > 1 ? "es" : "" ) );
        searchInfoLine->hideGauge();
        // De-activate the stop button
        stopButton->setEnabled( false );
    }
    else {
        // Search in progress
        searchInfoLine->setText(
                tr("Search in progress (%1 %)... %2 match%3 found so far.")
                .arg( progress )
                .arg( nbMatches )
                .arg( nbMatches > 1 ? "es" : "" ) );
        searchInfoLine->displayGauge( progress );
    }

    // Recompute the content of the filtered window.
    filteredView->updateData();

    // Also update the top window for the coloured bullets.
    update();
}

void CrawlerWidget::jumpToMatchingLine(int filteredLineNb)
{
    int mainViewLine = logFilteredData_->getMatchingLineNumber(filteredLineNb);
    logMainView->selectAndDisplayLine(mainViewLine);  // FIXME: should be done with a signal.
}

void CrawlerWidget::applyConfiguration()
{
    QFont font = Config().mainFont();

    LOG(logDEBUG) << "CrawlerWidget::applyConfiguration";

    logMainView->setFont(font);
    filteredView->setFont(font);

    logMainView->updateDisplaySize();
    logMainView->update();
    filteredView->updateDisplaySize();
    filteredView->update();

    // Update the SearchLine (history)
    updateSearchCombo();
}

void CrawlerWidget::loadingFinishedHandler( bool success )
{
    // FIXME, handle topLine
    // logMainView->updateData( logData_, topLine );
    logMainView->updateData();
    searchButton->setEnabled( true );

    emit loadingFinished( success );
}

void CrawlerWidget::fileChangedHandler( LogData::MonitoredFileStatus status )
{
    if ( ( status == LogData::Truncated ) &&
         !( searchInfoLine->text().isEmpty()) ) {
        logFilteredData_->clearSearch();
        filteredView->updateData();
        searchInfoLine->setText(
                tr("File truncated on disk, previous search results are not valid anymore.") );
    }
}

void CrawlerWidget::displayQuickFindBar()
{
    LOG(logDEBUG) << "CrawlerWidget::displayQuickFindBar";

    // Remember who had the focus
    qfSavedFocus_ = QApplication::focusWidget();

    quickFindWidget_->activate();
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
        // Activate the stop button
        stopButton->setEnabled( true );

        QRegExp regexp( searchText );
        // Start a new asynchronous search
        logFilteredData_->runSearch( regexp );
    } else {
        logFilteredData_->clearSearch();
        searchInfoLine->setText( "" );
        filteredView->updateData();
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
