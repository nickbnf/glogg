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

#include "crawlerwidget.h"

#include "configuration.h"

// Constructor makes all the child widgets and set up connections.
CrawlerWidget::CrawlerWidget(SavedSearches* searches, QWidget *parent)
        : QSplitter(parent)
{
    setOrientation(Qt::Vertical);

    // Initialise internal data (with empty file and search)
    logData_         = new LogData();
    logFilteredData_ = logData_->getNewFilteredData();

    logMainView  = new LogMainView( logData_ );
    bottomWindow = new QWidget;
    filteredView = new FilteredView( logFilteredData_ );

    savedSearches = searches;

    // Construct the Search Info line
    searchInfoLine = new InfoLine();
    searchInfoLine->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
    searchInfoLine->setLineWidth( 1 );

    // Construct the Search line
    searchLabel = new QLabel(tr("&Text: "));
    searchLineEdit = new QComboBox;
    searchLineEdit->setEditable( true );
    searchLineEdit->setCompleter( 0 );
    searchLineEdit->addItems( savedSearches->recentSearches() );
    searchLineEdit->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Minimum ) );
    searchLabel->setBuddy( searchLineEdit );
    searchButton = new QPushButton( tr("&Search") );

    QHBoxLayout* searchLineLayout = new QHBoxLayout;
    searchLineLayout->addWidget(searchLabel);
    searchLineLayout->addWidget(searchLineEdit);
    searchLineLayout->addWidget(searchButton);
    searchLineLayout->setContentsMargins(6, 0, 6, 0);

    // Construct the bottom window
    QVBoxLayout* bottomMainLayout = new QVBoxLayout;
    bottomMainLayout->addLayout(searchLineLayout);
    bottomMainLayout->addWidget(searchInfoLine);
    bottomMainLayout->addWidget(filteredView);
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

    connect(logMainView, SIGNAL( newSelection( int ) ),
            logMainView, SLOT( update() ) );
    connect(filteredView, SIGNAL( newSelection( int ) ),
            this, SLOT( jumpToMatchingLine( int ) ) );
    connect(filteredView, SIGNAL( newSelection( int ) ),
            filteredView, SLOT( update() ) );

    connect( logFilteredData_, SIGNAL( searchProgressed( int, int ) ),
            this, SLOT( updateFilteredView( int, int ) ) );

    // Sent load file update to MainWindow (for status update)
    connect( logData_, SIGNAL( loadingProgressed( int ) ),
            this, SIGNAL( loadingProgressed( int ) ) );
    connect( logData_, SIGNAL( loadingFinished() ),
            this, SLOT( loadingFinishedHandler() ) );
    connect( logData_, SIGNAL( fileChanged( LogData::MonitoredFileStatus ) ),
            this, SLOT( fileChangedHandler( LogData::MonitoredFileStatus ) ) );
}

// Start the asynchronous loading of a file.
bool CrawlerWidget::readFile( const QString& fileName, int topLine )
{
    // First we cancel any in progress search and loading
    logFilteredData_->interruptSearch();
    logData_->interruptLoading();

    QFileInfo fileInfo( fileName );
    if ( fileInfo.isReadable() )
    {
        // Means the file exist, so we invalidate the search
        // and redraw the screen.
        logData_->attachFile( fileName );
        replaceCurrentSearch( "" );
        logMainView->updateData( logData_, 0 );
        return true;
    }
    else {
        return false;
    }
}

void CrawlerWidget::getFileInfo( int* fileSize, int* fileNbLine,
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
    return logData_->getLineString( logMainView->getSelection() );
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

    filteredView->updateData();
}

void CrawlerWidget::jumpToMatchingLine(int filteredLineNb)
{
    int mainViewLine = logFilteredData_->getMatchingLineNumber(filteredLineNb);
    logMainView->displayLine(mainViewLine);  // FIXME: should be done with a signal.
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

void CrawlerWidget::loadingFinishedHandler()
{
    // FIXME, handle topLine
    // logMainView->updateData( logData_, topLine );
    logMainView->updateData();

    emit loadingFinished();
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

//
// Private functions
//

// Create a new search using the text passed, replace the currently
// used one and destroy the old one.
void CrawlerWidget::replaceCurrentSearch( const QString& searchText )
{
    // Interrupt the search if it's ongoing
    logFilteredData_->interruptSearch();

    if ( !searchText.isEmpty() ) {
        QRegExp regexp( searchText );
        // Start a new asynchronous search
        logFilteredData_->runSearch( regexp );
    } else {
        // We have to wait for the last search update (100%)
        // before clearing to avoid having remaining results.

        // FIXME: this is a bit of a hack, we call processEvents
        // for Qt to empty its event queue, including (hopefully)
        // the search update event sent by logFilteredData_. It saves
        // us the overhead of having proper sync.
        QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );

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
