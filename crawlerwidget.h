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

#ifndef CRAWLERWIDGET_H
#define CRAWLERWIDGET_H

#include <QLabel>
#include <QSplitter>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "logmainview.h"
#include "filteredview.h"

#include "logdata.h"
#include "logfiltereddata.h"

// Implements the central widget of the application.
// It includes both windows, the search line, the info
// lines and various buttons.
class CrawlerWidget : public QSplitter
{
  Q_OBJECT

  public:
    CrawlerWidget( QWidget *parent=0 );

    // Loads the passed file and reports success.
    bool readFile( const QString& fileName );
    // Get the size (in bytes) and number of lines in the current file.
    void getFileInfo( int* fileSize, int* fileNbLine );

  private slots:
    // Instructs the widget to start a search using the current search line.
    void startNewSearch();
    // Instructs the widget to reconfigure itself because Config() has changed.
    void applyConfiguration();
    // Called when new data must be displayed in the filtered window.
    void updateFilteredView();
    // Called when a new line has been selected in the filtered view,
    // to instruct the main view to jump to the matching line.
    void jumpToMatchingLine(int filteredLineNb);

  private:
    void replaceCurrentSearch( const QString& searchText );

    LogMainView*    logMainView;
    QWidget*        bottomWindow;
    QLabel*         searchLabel;
    QLineEdit*      searchLineEdit;
    QPushButton*    searchButton;
    FilteredView*   filteredView;
    QLabel*         searchInfoLine;

    QVBoxLayout*    bottomMainLayout;
    QHBoxLayout*    searchLineLayout;

    static LogData  emptyLogData;
    static LogFilteredData  emptyLogFilteredData;

    LogData*        logData_;
    LogFilteredData* logFilteredData_;

    qint64          logFileSize_;
};

#endif
