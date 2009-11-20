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

class LogData;

// Class encapsulating a single matching line
// Contains the line number the line was found in and its content.
class MatchingLine {
  public:
    MatchingLine( int line, QString str ) { lineNumber_ = line; lineString_ = str; };

    // Accessors
    int lineNumber() const { return lineNumber_; }
    QString lineContent() const { return lineString_; }

  private:
    int lineNumber_;
    QString lineString_;
};

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

    // Starts the search, sending newDataAvailable() when new data found
    void runSearch( const QRegExp& regExp );
    // Clear the search and the list of results.
    void clearSearch();
    // Returns the line number in the original LogData where the element
    // 'index' was found.
    int getMatchingLineNumber( int index ) const;
    // Returns weither the line number passed is in our list of matching ones.
    bool isLineInMatchingList( int lineNumber );

  signals:
    // Sent when the search has progressed, give the number of matches (so far)
    // and the percentage of completion
    void searchProgressed( int nbMatches, int progress );

  private:
    QString doGetLineString( int line ) const;
    int doGetNbLine() const;
    int doGetMaxLength() const;

    QList<MatchingLine> matchingLineList;

    const LogData* sourceLogData;
    QRegExp currentRegExp_;
    bool searchDone_;
    int maxLength_;
};

#endif

