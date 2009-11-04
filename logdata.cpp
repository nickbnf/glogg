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

// Constructs from a file.
LogData::LogData( const QString& fileName ) : AbstractLogData(), linePosition_()
{
    file_ = new QFile( fileName );

    if ( !file_->open( QIODevice::ReadOnly | QIODevice::Text ) )
        LOG(logERROR) << "Cannot open file " << fileName.toStdString();

    LOG(logDEBUG) << "Counting the lines...";

    // Count the number of lines and max length
    // (read big chunks to speed up reading from disk)
    int line_number = 0;
    while ( !file_->atEnd() ) {
        // Read a chunk of 5MB
        const qint64 block_beginning = file_->pos();
        const QByteArray block = file_->read( 5*1024*1024 );

        // Count the number of lines in each chunk
        int end = 0, pos = 0;
        while ( (end = block.indexOf("\n", pos)) != -1 ) {
            const int length = end-pos;
            const QString string = QString( block.mid(pos, length) );
            if ( length > maxLength_ )
                maxLength_ = length;
            linePosition_.append( block_beginning + pos );
            pos = end+1;
            ++line_number;
        }
    }
    nbLines_ = line_number;

    LOG(logDEBUG) << "... found " << nbLines_;
}

LogData::~LogData()
{
    delete file_;
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
    if ( line >= nbLines_ ) { return ""; /* exception? */ }

    file_->seek( linePosition_[line] );
    QString string = QString( file_->readLine() );

    return string;
}

// Return an initialized LogFilteredData. The search is not started.
LogFilteredData* LogData::getNewFilteredData( QRegExp& regExp ) const
{
    LogFilteredData* newFilteredData = new LogFilteredData( this, regExp );

    return newFilteredData;
}
