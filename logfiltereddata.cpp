/*
 * Copyright (C) 2009, 2010, 2011 Nicolas Bonnefon and other contributors
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

// This file implements LogFilteredData
// It stores a pointer to the LogData that created it,
// so should always be destroyed before the LogData.

#include "log.h"

#include <QString>
#include <cassert>

#include "utils.h"
#include "logdata.h"
#include "marks.h"
#include "logfiltereddata.h"

// Creates an empty set. It must be possible to display it without error.
// FIXME
LogFilteredData::LogFilteredData() : AbstractLogData()
{
    matchingLineList = QList<MatchingLine>();
    /* Prevent any more searching */
    maxLength_ = 0;
    searchDone_ = true;
    visibility_ = MarksAndMatches;

    workerThread_ = new LogFilteredDataWorkerThread( NULL );
    marks_ = new Marks();
}

// Usual constructor: just copy the data, the search is started by runSearch()
LogFilteredData::LogFilteredData( const LogData* logData )
    : AbstractLogData(), currentRegExp_()
{
    // Starts with an empty result list
    matchingLineList = SearchResultArray();
    maxLength_ = 0;
    nbLinesProcessed_ = 0;

    sourceLogData_ = logData;

    searchDone_ = false;

    visibility_ = MarksAndMatches;

    workerThread_ = new LogFilteredDataWorkerThread( logData );
    marks_ = new Marks();
    //
    // Forward the update signal
    connect( workerThread_, SIGNAL( searchProgressed( int, int ) ),
            this, SLOT( handleSearchProgressed( int, int ) ) );

    // Starts the worker thread
    workerThread_->start();
}

//
// Public functions
//

// Run the search and send newDataAvailable() signals.
void LogFilteredData::runSearch( const QRegExp& regExp )
{
    LOG(logDEBUG) << "Entering runSearch";

    // Reset the search
    currentRegExp_ = regExp;
    matchingLineList.clear();
    maxLength_ = 0;

    workerThread_->search( currentRegExp_ );
}

void LogFilteredData::updateSearch()
{
    LOG(logDEBUG) << "Entering updateSearch";

    workerThread_->updateSearch( currentRegExp_, nbLinesProcessed_ );
}

void LogFilteredData::interruptSearch()
{
    LOG(logDEBUG) << "Entering interruptSearch";

    workerThread_->interrupt();
}

void LogFilteredData::clearSearch()
{
    currentRegExp_ = QRegExp();
    matchingLineList.clear();
    maxLength_ = 0;
}

qint64 LogFilteredData::getMatchingLineNumber( int matchNum ) const
{
    qint64 matchingLine = matchingLineList[matchNum].lineNumber();

    return matchingLine;
}

// Scan the list for the 'lineNumber' passed
bool LogFilteredData::isLineInMatchingList( qint64 lineNumber )
{
    int index;                                    // Not used
    return lookupLineNumber<SearchResultArray>(
            matchingLineList, lineNumber, &index);
}

// Delegation to our Marks object

void LogFilteredData::addMark( qint64 line, QChar mark )
{
    assert( marks_ );

    marks_->addMark( line, mark );
}

qint64 LogFilteredData::getMark( QChar mark ) const
{
    assert( marks_ );

    return marks_->getMark( mark );
}

bool LogFilteredData::isLineMarked( qint64 line ) const
{
    assert( marks_ );

    return marks_->isLineMarked( line );
}

void LogFilteredData::deleteMark( QChar mark )
{
    assert( marks_ );

    marks_->deleteMark( mark );
}

void LogFilteredData::deleteMark( qint64 line )
{
    assert( marks_ );

    marks_->deleteMark( line );
}

void LogFilteredData::clearMarks()
{
    assert( marks_ );

    marks_->clear();
}

void LogFilteredData::setVisibility( Visibility visi )
{
    visibility_ = visi;
}

//
// Slots
//
void LogFilteredData::handleSearchProgressed( int nbMatches, int progress )
{
    LOG(logDEBUG) << "LogFilteredData::handleSearchProgressed matches="
        << nbMatches << " progress=" << progress;

    // searchDone_ = true;
    workerThread_->getSearchResult( &maxLength_, &matchingLineList, &nbLinesProcessed_ );
    filteredItemsCacheDirty_ = true;

    emit searchProgressed( nbMatches, progress );
}

qint64 LogFilteredData::findLogDataLine( qint64 lineNum ) const
{
    qint64 line;
    if ( visibility_ == MatchesOnly ) {
        if ( lineNum < matchingLineList.size() ) {
            line = matchingLineList[lineNum].lineNumber();
        }
        else {
            LOG(logERROR) << "Index too big in LogFilteredData: " << lineNum;
        }
    }
    else if ( visibility_ == MarksOnly ) {
        if ( lineNum < marks_->size() )
            line = marks_->getLineMarkedByIndex( lineNum );
        else
            LOG(logERROR) << "Index too big in LogFilteredData: " << lineNum;
    }
    else {
        // Regenerate the cache if needed
        if ( filteredItemsCacheDirty_ )
            regenerateFilteredItemsCache();

        if ( lineNum < filteredItemsCache_.size() )
            line = filteredItemsCache_[ lineNum ].lineNumber();
        else
            LOG(logERROR) << "Index too big in LogFilteredData: " << lineNum;
    }

    return line;
}

// Implementation of the virtual function.
QString LogFilteredData::doGetLineString( qint64 lineNum ) const
{
    qint64 line = findLogDataLine( lineNum );

    QString string = sourceLogData_->getLineString( line );
    return string;
}

// Implementation of the virtual function.
QString LogFilteredData::doGetExpandedLineString( qint64 lineNum ) const
{
    qint64 line = findLogDataLine( lineNum );

    QString string = sourceLogData_->getExpandedLineString( line );
    return string;
}

// Implementation of the virtual function.
QStringList LogFilteredData::doGetLines( qint64 first_line, int number ) const
{
    QStringList list;

    for ( int i = first_line; i < first_line + number; i++ ) {
        list.append( doGetLineString( i ) );
    }

    return list;
}

// Implementation of the virtual function.
QStringList LogFilteredData::doGetExpandedLines( qint64 first_line, int number ) const
{
    QStringList list;

    for ( int i = first_line; i < first_line + number; i++ ) {
        list.append( doGetExpandedLineString( i ) );
    }

    return list;
}

// Implementation of the virtual function.
qint64 LogFilteredData::doGetNbLine() const
{
    qint64 nbLines;

    if ( visibility_ == MatchesOnly )
        nbLines = matchingLineList.size();
    else if ( visibility_ == MarksOnly )
        nbLines = marks_->size();
    else
        // FIXME
        nbLines = matchingLineList.size() + marks_->size();

    return nbLines;
}

// Implementation of the virtual function.
int LogFilteredData::doGetMaxLength() const
{
    // FIXME
    return maxLength_;
}

// Implementation of the virtual function.
int LogFilteredData::doGetLineLength( qint64 lineNum ) const
{
    qint64 line = findLogDataLine( lineNum );
    return sourceLogData_->getExpandedLineString( line ).length();
}

void LogFilteredData::regenerateFilteredItemsCache() const
{
    LOG(logDEBUG) << "LogFilteredData::regenerateFilteredItemsCache";

    filteredItemsCache_.clear();
    filteredItemsCache_.reserve( matchingLineList.size() + marks_->size() );
    // (it's an overestimate but probably not by much so it's fine)

    QList<MatchingLine>::const_iterator i = matchingLineList.begin();
    Marks::const_iterator j = marks_->begin();

    while ( ( i != matchingLineList.end() ) || ( j != marks_->end() ) ) {
        qint64 next_mark =
            ( j != marks_->end() ) ? j->lineNumber() : std::numeric_limits<qint64>::max();
        qint64 next_match =
            ( i != matchingLineList.end() ) ? i->lineNumber() : std::numeric_limits<qint64>::max();
        // We choose a Mark over a Match if a line is both, just an arbitrary choice really.
        if ( next_mark <= next_match )
            filteredItemsCache_.append( FilteredItem( next_mark ) );
        else
            filteredItemsCache_.append( FilteredItem( next_match ) );
        if ( i != matchingLineList.end() )
            ++i;
        if ( j != marks_->end() )
            ++j;
    }

    filteredItemsCacheDirty_ = false;
}
