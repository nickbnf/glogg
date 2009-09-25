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

#include "crawlerwidget.h"

#include "configuration.h"

// Construct an empty set of data, and of filtered data to use as a default
LogData         CrawlerWidget::emptyLogData;
LogFilteredData CrawlerWidget::emptyLogFilteredData;

// Constructor makes all the child widgets and set up connections.
CrawlerWidget::CrawlerWidget(QWidget *parent) : QSplitter(parent)
{
    setOrientation(Qt::Vertical);

    // Initialize internal data
    logData         = &emptyLogData;
    logFilteredData = &emptyLogFilteredData;

    logMainView =  new LogMainView( logData );
    bottomWindow = new QWidget;
    filteredView = new FilteredView( logFilteredData );

    // Construct the Search Info line
    searchInfoLine = new QLabel();
    searchInfoLine->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
    searchInfoLine->setLineWidth( 1 );

    // Construct the Search line
    searchLabel =  new QLabel(tr("&Text: "));
    searchLineEdit = new QLineEdit;
    searchLabel->setBuddy(searchLineEdit);
    searchButton = new QPushButton(tr("&Search"));
    searchButton->setEnabled(false);

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
    connect(searchLineEdit, SIGNAL(returnPressed()),
            searchButton, SIGNAL(clicked()));
    connect(searchButton, SIGNAL( clicked() ),
            this, SLOT( createNewSearch() ) );
    connect(logMainView, SIGNAL( newSelection( int ) ),
            logMainView, SLOT( update() ) );
    connect(filteredView, SIGNAL( newSelection( int ) ),
            this, SLOT( jumpToMatchingLine( int ) ) );
    connect(filteredView, SIGNAL( newSelection( int ) ),
            filteredView, SLOT( update() ) );
}

bool CrawlerWidget::readFile(const QString& fileName)
{
    QFile file(fileName);

    if (file.open(QFile::ReadOnly | QFile::Text)) {
        LogData* oldLogData = logData;
        logData = new LogData(file.readAll());

        logMainView->updateData(logData);

        if (oldLogData != &emptyLogData)
            delete oldLogData;

        return true;
    }
    else {
        return false;
    }
}

//
// Slots
//

void CrawlerWidget::createNewSearch()
{
    // Delete (and disconnect the object)
    if ( logFilteredData != &emptyLogFilteredData )
        delete logFilteredData;

    QString text = searchLineEdit->text();
    QRegExp regexp(text);

    if ( !text.isEmpty() ) {
        // Create the new LogFilteredData...
        logFilteredData = logData->getNewFilteredData( regexp );
        // ... and arrange to receive notification of updates
        connect( logFilteredData, SIGNAL( newDataAvailable() ),
                this, SLOT( updateFilteredView() ) );
        logFilteredData->runSearch();
    } else {
        logFilteredData = &emptyLogFilteredData;
        // We won't receive an event from the emptyLogFilteredData
        searchInfoLine->setText( "" );
        filteredView->updateData( logFilteredData );
    }
    // Connect the search to the top view
    logMainView->useNewFiltering( logFilteredData );
}

void CrawlerWidget::updateFilteredView()
{
    LOG(logDEBUG) << "updateFilteredView received.";

    searchInfoLine->setText( tr("Found %1 match%2.")
            .arg( logFilteredData->getNbLine() )
            .arg( logFilteredData->getNbLine() > 1 ? "es" : "" ) );
    filteredView->updateData(logFilteredData);
}

void CrawlerWidget::jumpToMatchingLine(int filteredLineNb)
{
    int mainViewLine = logFilteredData->getMatchingLineNumber(filteredLineNb);
    logMainView->displayLine(mainViewLine);  // FIXME: should be done with a signal.
}

void CrawlerWidget::applyConfiguration()
{
    QFont font = Config().mainFont();

    LOG(logDEBUG) << "CrawlerWidget::applyConfiguration";

    logMainView->setFont(font);
    filteredView->setFont(font);

    logMainView->update();
    filteredView->update();
}
