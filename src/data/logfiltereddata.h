/*
 * Copyright (C) 2009, 2010, 2011, 2012, 2017 Nicolas Bonnefon and other contributors
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

#include <memory>

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QStringList>
#include <QRegularExpression>

#include "abstractlogdata.h"
#include "logfiltereddataworkerthread.h"
#include "marks.h"

class LogData;
class Marks;

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

    ~LogFilteredData();

    // Starts the async search, sending newDataAvailable() when new data found.
    // If a search is already in progress this function will block until
    // it is done, so the application should call interruptSearch() first.
    void runSearch(const QRegularExpression &regExp );
    // Add to the existing search, starting at the line when the search was
    // last stopped. Used when the file on disk has been added too.
    void updateSearch();
    // Interrupt the running search if one is in progress.
    // Nothing is done if no search is in progress.
    void interruptSearch();
    // Clear the search and the list of results.
    void clearSearch();
    // Returns the line number in the original LogData where the element
    // 'index' was found.
    qint64 getMatchingLineNumber( int index ) const;
    // Returns whether the line number passed is in our list of matching ones.
    bool isLineInMatchingList( qint64 lineNumber );

    // Returns the line 'index' in filterd log data that matches
    // given original line number
    int getLineIndexNumber( quint64 lineNumber ) const;

    // Returns the number of lines in the source log data
    LineNumber getNbTotalLines() const;
    // Returns the number of matches (independently of the visibility)
    LineNumber getNbMatches() const;
    // Returns the number of marks (independently of the visibility)
    LineNumber getNbMarks() const;

    // Returns the reason why the line at the passed index is in the filtered
    // data.  It can be because it is either a mark or a match.
    enum FilteredLineType { Match, Mark };
    FilteredLineType filteredLineTypeByIndex( int index ) const;

    // Marks interface (delegated to a Marks object)

    // Add a mark at the given line, optionally identified by the given char
    // If a mark for this char already exist, the previous one is replaced.
    void addMark( qint64 line, QChar mark = QChar() );
    // Get the (unique) mark identified by the passed char.
    qint64 getMark( QChar mark ) const;
    // Returns wheither the passed line has a mark on it.
    bool isLineMarked( qint64 line ) const;
    // Get the first mark after the line passed (-1 if none)
    qint64 getMarkAfter( qint64 line ) const;
    // Get the first mark before the line passed (-1 if none)
    qint64 getMarkBefore( qint64 line ) const;
    // Delete the mark identified by the passed char.
    void deleteMark( QChar mark );
    // Delete the mark present on the passed line or do nothing if there is
    // none.
    void deleteMark( qint64 line );
    // Completely clear the marks list.
    void clearMarks();

    // Changes what the AbstractLogData returns via its getXLines/getNbLines
    // API.
    enum Visibility { MatchesOnly, MarksOnly, MarksAndMatches };
    void setVisibility( Visibility visibility );

    QRegularExpression getCurrentRegExp() const { return currentRegExp_;}

  signals:
    // Sent when the search has progressed, give the number of matches (so far)
    // and the percentage of completion
    // Also include the initial position to allow the client to distinguish
    // between full and partial searches
    void searchProgressed( int nbMatches, int progress, qint64 initial_position );

  private slots:
    void handleSearchProgressed( int NbMatches, int progress, qint64 initial_position );

  private:
    class FilteredItem;

    // Implementation of virtual functions
    QString doGetLineString( qint64 line ) const;
    QString doGetExpandedLineString( qint64 line ) const;
    QStringList doGetLines( qint64 first, int number ) const;
    QStringList doGetExpandedLines( qint64 first, int number ) const;
    qint64 doGetNbLine() const;
    int doGetMaxLength() const;
    int doGetLineLength( qint64 line ) const;
    void doSetDisplayEncoding( Encoding encoding );
    void doSetMultibyteEncodingOffsets( int before_cr, int after_cr ) override;

    // List of the matching line numbers
    SearchResultArray matching_lines_;

    const LogData* sourceLogData_;
    QRegularExpression currentRegExp_;
    bool searchDone_;
    int maxLength_;
    int maxLengthMarks_;
    // Number of lines of the LogData that has been searched for:
    qint64 nbLinesProcessed_;

    Visibility visibility_;

    // Cache used to combine Marks and Matches
    // when visibility_ == MarksAndMatches
    // (QVector store actual objects instead of pointers)
    mutable std::vector<FilteredItem> filteredItemsCache_;
    mutable bool filteredItemsCacheDirty_;

    LogFilteredDataWorkerThread workerThread_;
    Marks marks_;

    // Utility functions
    LineNumber findLogDataLine( LineNumber lineNum ) const;
    LineNumber findFilteredLine( LineNumber lineNum ) const;

    void regenerateFilteredItemsCache() const;
};

// A class representing a Mark or Match.
// Conceptually it should be a base class for Mark and MatchingLine,
// but we implement it this way for performance reason as we create plenty of
// those everytime we refresh the cache.
// Specifically it allows to store this in the cache by value instead
// of pointer (less small allocations and no RTTI).
class LogFilteredData::FilteredItem {
  public:
    // A default ctor seems to be necessary for QVector
    FilteredItem()
    { lineNumber_ = 0; }
    FilteredItem( LineNumber lineNumber, FilteredLineType type )
    { lineNumber_ = lineNumber; type_ = type; }

    LineNumber lineNumber() const
    { return lineNumber_; }
    FilteredLineType type() const
    { return type_; }

    bool operator <( const LogFilteredData::FilteredItem& other ) const
    { return lineNumber_ < other.lineNumber_; }

    bool operator <( const LineNumber& lineNumber ) const
    { return lineNumber_ < lineNumber; }

  private:
    LineNumber lineNumber_;
    FilteredLineType type_;
};

#endif
