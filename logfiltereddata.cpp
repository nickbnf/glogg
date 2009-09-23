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

// This file implements LogFilteredData
// It stores a pointer to the internal data of the LogData that created it,
// so should ialways be destroyed before the LogData.
// The implementation depends heavily on the implementation of LogData, that
// should probably be fixed!

#include "log.h"

#include <QString>

#include "logfiltereddata.h"

// Creates an empty set. It must be possible to display it without error.
LogFilteredData::LogFilteredData() : AbstractLogData()
{
    matchingLineList = QList<matchingLine>();
    /* Prevent any more searching */
    searchDone_ = true;
}

// Usual constructor: just copy the data, the search is started by runSearch()
LogFilteredData::LogFilteredData( QStringList* logData, QRegExp regExp )
    : AbstractLogData()
{
    matchingLineList = QList<matchingLine>();

    if ( logData != NULL ) {
        sourceLogData = logData;
        currentRegExp = regExp;

        searchDone_ = false;
    } else {
        // Empty set
        searchDone_ = true;
    }
}

// Run the search and send newDataAvailable() signals.
void LogFilteredData::runSearch()
{
    LOG(logDEBUG) << "Entering runSearch";

    if ( !searchDone_ ) {
        int lineNum = 0;
        for ( QStringList::iterator i = sourceLogData->begin();
                i != sourceLogData->end();
                ++i, ++lineNum ) {
            if ( currentRegExp.indexIn( *i ) != -1 ) {
                const int length = i->length();
                if ( length > maxLength )
                    maxLength = length;
                matchingLine match( lineNum, *i );
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
