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
// It stores a pointer to the internal data of the LogData that created it,
// so should ialways be destroyed before the LogData.
// The implementation depends heavily on the implementation of LogData, that
// should probably be fixed!

#include "log.h"

#include <QString>

#include "logdata.h"
#include "logfiltereddata.h"

// Creates an empty set. It must be possible to display it without error.
LogFilteredData::LogFilteredData() : AbstractLogData()
{
    matchingLineList = QList<MatchingLine>();
    /* Prevent any more searching */
    maxLength_ = 0;
    searchDone_ = true;
}

// Usual constructor: just copy the data, the search is started by runSearch()
LogFilteredData::LogFilteredData( const LogData* logData, const QRegExp& regExp )
    : AbstractLogData(), currentRegExp( regExp )
{
    matchingLineList = QList<MatchingLine>();
    maxLength_ = 0;

    sourceLogData = logData;

    searchDone_ = false;
}

// Run the search and send newDataAvailable() signals.
void LogFilteredData::runSearch()
{
    LOG(logDEBUG) << "Entering runSearch";

    if ( !searchDone_ ) {
        for ( int i = 0; i < sourceLogData->getNbLine(); i++ ) {
            const QString line = sourceLogData->getLineString( i );
            if ( currentRegExp.indexIn( line ) != -1 ) {
                const int length = line.length();
                if ( length > maxLength_ )
                    maxLength_ = length;
                MatchingLine match( i, line );
                matchingLineList.append( match );
            }
        }

        searchDone_ = true;
        emit newDataAvailable();
    }

    LOG(logDEBUG) << "End runSearch";
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
