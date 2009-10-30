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

// This file implements the CrawlerWidget class.
// It is responsible for creating and managing the two views and all
// the UI elements.  It implements the connection between the UI elements.
// It also owns the sets of data (full and filtered).

#include "log.h"

#include <Qt>
#include <QFile>
#include <QLineEdit>

#include "crawlerwidget.h"

#include "configuration.h"

// Construct an empty set of data, and of filtered data to use as a default
LogData         CrawlerWidget::emptyLogData;
LogFilteredData CrawlerWidget::emptyLogFilteredData;

// Constructor makes all the child widgets and set up connections.
CrawlerWidget::CrawlerWidget(SavedSearches* searches, QWidget *parent)
        : QSplitter(parent)
{
    setOrientation(Qt::Vertical);

    // Initialize internal data
    logData_         = &emptyLogData;
    logFilteredData_ = &emptyLogFilteredData;

    logFileSize_     = 0;

    logMainView  = new LogMainView( logData_ );
    bottomWindow = new QWidget;
    filteredView = new FilteredView( logFilteredData_ );

    savedSearches = searches;

    // Construct the Search Info line
    searchInfoLine = new QLabel();
    searchInfoLine->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
    searchInfoLine->setLineWidth( 1 );

    // Construct the Search line
    searchLabel = new QLabel(tr("&Text: "));
    searchLineEdit = new QComboBox;
    searchLineEdit->setEditable( true );
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
}

bool CrawlerWidget::readFile( const QString& fileName, int topLine )
{
    QFile file(fileName);

    if ( file.open(QFile::ReadOnly | QFile::Text) ) {
        // Start an empty search (will use the empty LFD)
        replaceCurrentSearch( "" );

        LogData* oldLogData = logData_;
        logData_ = new LogData( file.readAll() );

        logMainView->updateData( logData_, topLine );

        logFileSize_ = file.size();

        file.close();

        if (oldLogData != &emptyLogData)
            delete oldLogData;

        return true;
    }
    else {
        return false;
    }
}

void CrawlerWidget::getFileInfo( int* fileSize, int* fileNbLine ) const
{
    *fileSize = logFileSize_;
    *fileNbLine = logData_->getNbLine();
}

// The top line is first one on the main display
int CrawlerWidget::getTopLine() const
{
    return logMainView->getTopLine();
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

void CrawlerWidget::updateFilteredView()
{
    LOG(logDEBUG) << "updateFilteredView received.";

    searchInfoLine->setText( tr("Found %1 match%2.")
            .arg( logFilteredData_->getNbLine() )
            .arg( logFilteredData_->getNbLine() > 1 ? "es" : "" ) );
    filteredView->updateData(logFilteredData_, 0);
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

//
// Private functions
//

// Create a new search using the text passed, replace the currently
// used one and destroy the old one.
void CrawlerWidget::replaceCurrentSearch( const QString& searchText )
{
    // Delete (and disconnect the object)
    if ( logFilteredData_ != &emptyLogFilteredData )
        delete logFilteredData_;

    if ( !searchText.isEmpty() ) {
        QRegExp regexp(searchText);
        // Create the new LogFilteredData...
        logFilteredData_ = logData_->getNewFilteredData( regexp );
        // ... and arrange to receive notification of updates
        connect( logFilteredData_, SIGNAL( newDataAvailable() ),
                this, SLOT( updateFilteredView() ) );
        logFilteredData_->runSearch();
    } else {
        logFilteredData_ = &emptyLogFilteredData;
        // We won't receive an event from the emptyLogFilteredData
        searchInfoLine->setText( "" );
        filteredView->updateData( logFilteredData_, 0 );
    }
    // Connect the search to the top view
    logMainView->useNewFiltering( logFilteredData_ );
}

// Updates the content of the drop down list for the saved searches,
// called when the SavedSearch has been changed.
void CrawlerWidget::updateSearchCombo()
{
    searchLineEdit->clear();
    searchLineEdit->addItems( savedSearches->recentSearches() );
}
