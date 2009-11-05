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

#include "common.h"
#include "logdata.h"

// Constructs an empty log file.
// It must be displayed without error.
LogData::LogData() : AbstractLogData(), linePosition_()
{
    file_      = NULL;
    fileSize_  = 0;
    nbLines_   = 0;
    maxLength_ = 0;
}

LogData::~LogData()
{
    if ( file_ )
        delete file_;
}

//
// Public functions
//

bool LogData::attachFile( const QString& fileName )
{
    file_ = new QFile( fileName );

    if ( !file_->open( QIODevice::ReadOnly ) )
    {
        // TODO: Check that the file is seekable?
        LOG(logERROR) << "Cannot open file " << fileName.toStdString();
        return false;
    }

    fileSize_ = file_->size();

    LOG(logDEBUG) << "Counting the lines...";

    // Count the number of lines and max length
    // (read big chunks to speed up reading from disk)
    int end = 0, pos = 0;
    while ( !file_->atEnd() ) {
        // Read a chunk of 5MB
        const qint64 block_beginning = file_->pos();
        const QByteArray block = file_->read( 5*1024*1024 );

        // Count the number of lines in each chunk
        qint64 next_lf = 0;
        while ( next_lf != -1 ) {
            const qint64 pos_within_block = max2( pos - block_beginning, 0LL);
            next_lf = block.indexOf( "\n", pos_within_block );
            if ( next_lf != -1 ) {
                end = next_lf + block_beginning;
                const int length = end-pos;
                if ( length > maxLength_ )
                    maxLength_ = length;
                pos = end+1;
                linePosition_.append( pos );
            }
        }
    }
    nbLines_ = linePosition_.size();

    LOG(logDEBUG) << "... found " << nbLines_ << " lines.";

    return true;
}

qint64 LogData::getFileSize() const
{
    return fileSize_;
}

// Return an initialized LogFilteredData. The search is not started.
LogFilteredData* LogData::getNewFilteredData( QRegExp& regExp ) const
{
    LogFilteredData* newFilteredData = new LogFilteredData( this, regExp );

    return newFilteredData;
}

//
// Implementation of virtual functions
//
int LogData::doGetNbLine() const
{
    return nbLines_;
}

int LogData::doGetMaxLength() const
{
    return maxLength_;
}

QString LogData::doGetLineString( int line ) const
{
    if ( line >= nbLines_ ) { return ""; /* exception? */ }

    file_->seek( linePosition_[line] );
    QString string = QString( file_->readLine() );
    string.chop( 1 );

    return string;
}

