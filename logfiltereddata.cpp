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

// This file implements LogFilteredData
// It stores a pointer to the LogData that created it,
// so should always be destroyed before the LogData.

#include "log.h"

#include <QString>

#include "logdata.h"
#include "logfiltereddata.h"

// Creates an empty set. It must be possible to display it without error.
// FIXME
LogFilteredData::LogFilteredData() : AbstractLogData(), workerThread_( NULL )
{
    matchingLineList = QList<MatchingLine>();
    /* Prevent any more searching */
    maxLength_ = 0;
    searchDone_ = true;
}

// Usual constructor: just copy the data, the search is started by runSearch()
LogFilteredData::LogFilteredData( const LogData* logData )
    : AbstractLogData(), currentRegExp_(), workerThread_( logData )
{
    // Starts with an empty result list
    matchingLineList = SearchResultArray();
    maxLength_ = 0;

    sourceLogData = logData;

    searchDone_ = false;

    // Forward the update signal
    connect( &workerThread_, SIGNAL( searchProgressed( int, int ) ),
            this, SLOT( handleSearchProgressed( int, int ) ) );

    // Starts the worker thread
    workerThread_.start();
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

    workerThread_.search( regExp );

}

void LogFilteredData::interruptSearch()
{
    LOG(logDEBUG) << "Entering interruptSearch";

    workerThread_.interrupt();
}

void LogFilteredData::clearSearch()
{
    currentRegExp_ = QRegExp();
    matchingLineList.clear();
    maxLength_ = 0;
}

int LogFilteredData::getMatchingLineNumber( int lineNum ) const
{
    int matchingNb = matchingLineList[lineNum].lineNumber();

    return matchingNb;
}

// Scan the list for the 'lineNumber' passed
bool LogFilteredData::isLineInMatchingList( int lineNumber )
{
    // Use a bisection method to find the number
    // (we know the list is sorted)

    int minIndex = 0;
    int maxIndex = matchingLineList.size() - 1;
    // If the list is not empty
    if ( maxIndex - minIndex >= 0 ) {
        // First we test the ends
        if (  ( matchingLineList[minIndex].lineNumber() == lineNumber )
                || ( matchingLineList[maxIndex].lineNumber() == lineNumber ) )
            return true;
        // Then we test the rest
        while ( (maxIndex - minIndex) > 1 ) {
            const int tryIndex = (minIndex + maxIndex) / 2;
            const int currentMatchingNumber =
                matchingLineList[tryIndex].lineNumber();
            if ( currentMatchingNumber > lineNumber )
                maxIndex = tryIndex;
            else if ( currentMatchingNumber < lineNumber )
                minIndex = tryIndex;
            else if ( currentMatchingNumber == lineNumber )
                return true;
        }
    }

    return false;
}

//
// Slots
//
void LogFilteredData::handleSearchProgressed( int nbMatches, int progress )
{
    LOG(logDEBUG) << "LogFilteredData::handleSearchProgressed matches="
        << nbMatches << " progress=" << progress;

    // searchDone_ = true;
    workerThread_.getSearchResult( &maxLength_, &matchingLineList );

    emit searchProgressed( nbMatches, progress );
}

// Implementation of the virtual function.
QString LogFilteredData::doGetLineString( int lineNum ) const
{
    QString string = matchingLineList[lineNum].lineContent();

    return string;
}

// Implementation of the virtual function.
int LogFilteredData::doGetNbLine() const
{
    return matchingLineList.size();
}

// Implementation of the virtual function.
int LogFilteredData::doGetMaxLength() const
{
    return maxLength_;
}
