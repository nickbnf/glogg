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

#ifndef CRAWLERWIDGET_H
#define CRAWLERWIDGET_H

#include <QSplitter>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "logmainview.h"
#include "filteredview.h"
#include "savedsearches.h"
#include "infoline.h"

#include "logdata.h"
#include "logfiltereddata.h"

// Implements the central widget of the application.
// It includes both windows, the search line, the info
// lines and various buttons.
class CrawlerWidget : public QSplitter
{
  Q_OBJECT

  public:
    CrawlerWidget( SavedSearches* searches, QWidget *parent=0 );

    // Loads the passed file and reports success.
    bool readFile( const QString& fileName, int topLine );
    // Get the size (in bytes) and number of lines in the current file.
    void getFileInfo( int* fileSize, int* fileNbLine ) const;
    // Get the line number of the first line displayed.
    int getTopLine() const;

  signals:
    // Sent to signal the client load has progressed,
    // passing the completion percentage.
    void loadingProgressed( int progress );
    void loadingFinished();

  private slots:
    // Instructs the widget to start a search using the current search line.
    void startNewSearch();
    // Instructs the widget to reconfigure itself because Config() has changed.
    void applyConfiguration();
    // Called when new data must be displayed in the filtered window.
    void updateFilteredView( int nbMatches, int progress );
    // Called when a new line has been selected in the filtered view,
    // to instruct the main view to jump to the matching line.
    void jumpToMatchingLine( int filteredLineNb );

    void loadingFinishedHandler();

  private:
    void replaceCurrentSearch( const QString& searchText );
    void updateSearchCombo();

    LogMainView*    logMainView;
    QWidget*        bottomWindow;
    QLabel*         searchLabel;
    QComboBox*      searchLineEdit;
    QPushButton*    searchButton;
    FilteredView*   filteredView;
    InfoLine*       searchInfoLine;

    QVBoxLayout*    bottomMainLayout;
    QHBoxLayout*    searchLineLayout;

    SavedSearches*  savedSearches;

    LogData*        logData_;
    LogFilteredData* logFilteredData_;

    qint64          logFileSize_;
};

#endif
