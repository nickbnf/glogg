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

#ifndef LOGFILTEREDDATA_H
#define LOGFILTEREDDATA_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QStringList>
#include <QRegExp>

#include "abstractlogdata.h"
#include "logfiltereddataworkerthread.h"

class LogData;

// A list of matches found in a LogData, it stores all the matching lines,
// which can be accessed using the AbstractLogData interface, together with
// the original line number where they were found.
// Constructing such objet does not start the search.
// This object should be constructed by a LogData.
class LogFilteredData : public AbstractLogData {
  Q_OBJECT

  public:
    // Creates an empty LogFilteredData
    LogFilteredData();
    // Constructor used by LogData
    LogFilteredData( const LogData* logData );

    // Starts the async search, sending newDataAvailable() when new data found.
    // If a search is already in progress this function will block until
    // it is done, so the application should call interruptSearch() first.
    void runSearch( const QRegExp& regExp );
    // Interrupt the running search if one is in progress.
    // Nothing is done if no search is in progress.
    void interruptSearch();
    // Clear the search and the list of results.
    void clearSearch();
    // Returns the line number in the original LogData where the element
    // 'index' was found.
    qint64 getMatchingLineNumber( int index ) const;
    // Returns weither the line number passed is in our list of matching ones.
    bool isLineInMatchingList( qint64 lineNumber );

  signals:
    // Sent when the search has progressed, give the number of matches (so far)
    // and the percentage of completion
    void searchProgressed( int nbMatches, int progress );

  private slots:
    void handleSearchProgressed( int NbMatches, int progress );

  private:
    // Implementation of virtual functions
    QString doGetLineString( qint64 line ) const;
    QStringList doGetLines( qint64 first, int number ) const;
    qint64 doGetNbLine() const;
    int doGetMaxLength() const;
    int doGetLineLength( qint64 line ) const;

    QList<MatchingLine> matchingLineList;

    const LogData* sourceLogData;
    QRegExp currentRegExp_;
    bool searchDone_;
    int maxLength_;

    LogFilteredDataWorkerThread workerThread_;
};

#endif

