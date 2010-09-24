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
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "logmainview.h"
#include "filteredview.h"
#include "savedsearches.h"
#include "infoline.h"
#include "quickfindpattern.h"
#include "quickfindwidget.h"

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
    // Stop the loading of the file if one is in progress
    void stopLoading();
    // Get the size (in bytes) and number of lines in the current file.
    void getFileInfo( qint64* fileSize, int* fileNbLine,
       QDateTime* lastModified ) const;
    // Get the line number of the first line displayed.
    int getTopLine() const;
    // Get the selected text as a string (from the main window)
    QString getSelectedText() const;

    // Display the QFB at the bottom, remembering where the focus was
    void displayQuickFindBar();

  protected:
    void keyPressEvent( QKeyEvent* keyEvent );

  signals:
    // Sent to signal the client load has progressed,
    // passing the completion percentage.
    void loadingProgressed( int progress );
    // Sent to the client when the loading has finished
    // weither succesfull or not.
    void loadingFinished( bool success );
    // Sent when follow mode is enabled/disabled
    void followSet( bool checked );
    // Sent up to the MainWindow to disable the follow mode
    void followDisabled();
    // Sent up when the current line number is updated
    void updateLineNumber( int line );

  private slots:
    // Instructs the widget to start a search using the current search line.
    void startNewSearch();
    // Stop the currently ongoing search (if one exists)
    void stopSearch();
    // Instructs the widget to reconfigure itself because Config() has changed.
    void applyConfiguration();
    // Called when new data must be displayed in the filtered window.
    void updateFilteredView( int nbMatches, int progress );
    // Called when a new line has been selected in the filtered view,
    // to instruct the main view to jump to the matching line.
    void jumpToMatchingLine( int filteredLineNb );

    void loadingFinishedHandler( bool success );
    // Manages the info lines to inform the user the file has changed.
    void fileChangedHandler( LogData::MonitoredFileStatus );

    void hideQuickFindBar();

    // Instructs the widget to apply the pattern (called by the QF widget).
    void applyNewQFPattern( const QString& newPattern );

    // Instructs the widget to change the pattern in the QuickFind widget
    // and confirm it.
    void changeQFPattern( const QString& newPattern );

    void searchForward();
    void searchBackward();

  private:
    void replaceCurrentSearch( const QString& searchText );
    void updateSearchCombo();
    AbstractLogView* searchableWidget() const;

    LogMainView*    logMainView;
    QWidget*        bottomWindow;
    QLabel*         searchLabel;
    QComboBox*      searchLineEdit;
    QToolButton*    searchButton;
    QToolButton*    stopButton;
    FilteredView*   filteredView;
    InfoLine*       searchInfoLine;
    QuickFindWidget* quickFindWidget_;

    QVBoxLayout*    bottomMainLayout;
    QHBoxLayout*    searchLineLayout;

    SavedSearches*  savedSearches;

    QuickFindPattern* quickFindPattern_;

    LogData*        logData_;
    LogFilteredData* logFilteredData_;

    qint64          logFileSize_;

    QWidget*        qfSavedFocus_;
};

#endif
