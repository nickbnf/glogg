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

// This file implements LogData, the content of a log file.

#include <iostream>

#include "log.h"

#include "logdata.h"

// Constructs an empty log file.
// It must be displayed without error.
LogData::LogData() : AbstractLogData()
{
    list_ = NULL;
    nbLines_ = 0;
    maxLength_ = 0;
}

// Constructs from a data chunk.
LogData::LogData( const QByteArray &byteArray ) : AbstractLogData()
{
    list_ = new QStringList();
    maxLength_ = 0;

    int pos=0, end=0;
    while ( (end = byteArray.indexOf("\n", pos)) != -1 ) {
        const int length = end-pos;
        const QString string = QString( byteArray.mid(pos, length) );
        if ( length > maxLength_ )
            maxLength_ = length;
        pos = end+1;
        list_->append( string );
    }
    nbLines_ = list_->size();

    LOG(logDEBUG) << "Found " << nbLines_ << " lines.";
}

int LogData::doGetNbLine() const
{
    return nbLines_;
}

int LogData::doGetMaxLength() const
{
    return maxLength_;
}

QString LogData::doGetLineString(int line) const
{
    if ( line >= nbLines_ ) { /* exception? */ }

    QString string = list_->at( line );

    return string;
}

// Return an initialized LogFilteredData. The search is not started.
LogFilteredData* LogData::getNewFilteredData( QRegExp& regExp ) const
{
    LogFilteredData* newFilteredData = new LogFilteredData( list_, regExp );

    return newFilteredData;
}
