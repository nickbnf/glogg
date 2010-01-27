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

#include <QFileInfo>

#include "log.h"

#include "common.h"
#include "logdata.h"

// Constructs an empty log file.
// It must be displayed without error.
LogData::LogData() : AbstractLogData(), fileWatcher_(),
    linePosition_(), fileMutex_(), workerThread_()
{
    // Start with an "empty" log
    file_         = NULL;
    fileSize_     = 0;
    nbLines_      = 0;
    maxLength_    = 0;

    // Initialise the file watcher
    connect( &fileWatcher_, SIGNAL( fileChanged( const QString& ) ),
            this, SLOT( fileChangedOnDisk() ) );
    // Forward the update signal
    connect( &workerThread_, SIGNAL( indexingProgressed( int ) ),
            this, SIGNAL( loadingProgressed( int ) ) );
    connect( &workerThread_, SIGNAL( indexingFinished( bool ) ),
            this, SLOT( indexingFinished( bool ) ) );

    // Starts the worker thread
    workerThread_.start();
}

LogData::~LogData()
{
    if ( file_ )
        delete file_;
}

//
// Public functions
//

void LogData::attachFile( const QString& fileName )
{
    if ( file_ ) {
        // Remove the current file from the watch list
        fileWatcher_.removeFile( file_->fileName() );
    }

    // Now the file is open, so we are committed to loading it
    // We invalidate the non indexed data
    fileSize_     = 0;
    nbLines_      = 0;
    maxLength_    = 0;

    // And instructs the worker thread to index the whole file asynchronously
    LOG(logDEBUG) << "Attaching " << fileName.toStdString();
    indexingFileName_ = fileName;
    workerThread_.attachFile( fileName );
    workerThread_.indexAll();

    // The client might now use the new file (even if it is empty for now)!
}

void LogData::interruptLoading()
{
    workerThread_.interrupt();
}

qint64 LogData::getFileSize() const
{
    return fileSize_;
}

QDateTime LogData::getLastModifiedDate() const
{
    return lastModifiedDate_;
}

// Return an initialised LogFilteredData. The search is not started.
LogFilteredData* LogData::getNewFilteredData() const
{
    LogFilteredData* newFilteredData = new LogFilteredData( this );

    return newFilteredData;
}

//
// Slots
//

void LogData::fileChangedOnDisk()
{
    LOG(logDEBUG) << "signalFileChanged";

    fileWatcher_.removeFile( file_->fileName() );

    const QString name = file_->fileName();
    QFileInfo info( name );

    {
        LOG(logDEBUG) << "current fileSize=" << fileSize_;
        LOG(logDEBUG) << "info file_->size()=" << info.size();
        if ( info.size() < fileSize_ ) {
            fileChangedOnDisk_ = Truncated;
            LOG(logINFO) << "File truncated";
            workerThread_.indexAll();
        }
        else if ( fileChangedOnDisk_ != DataAdded ) {
            fileChangedOnDisk_ = DataAdded;
            LOG(logINFO) << "New data on disk";
            workerThread_.indexAdditionalLines( fileSize_ );
        }
    }

    lastModifiedDate_ = info.lastModified();

    emit fileChanged( fileChangedOnDisk_ );
}

void LogData::indexingFinished( bool success )
{
    LOG(logDEBUG) << "indexingFinished: now copying in LogData.";

    // We use the newly created file data
    // (Qt implicit copy makes this fast!)
    workerThread_.getIndexingData( &fileSize_, &maxLength_, &linePosition_ );
    nbLines_ = linePosition_.size();

    LOG(logDEBUG) << "indexingFinished: " << success <<
        ", found " << nbLines_ << " lines.";

    if ( success )
    {
        if ( file_ ) {
            // Use the new filename
            file_->setFileName( indexingFileName_ );
        }
        else {
            file_ = new QFile( indexingFileName_ );
        }

        lastModifiedDate_ = QDateTime();
        QFileInfo fileInfo( indexingFileName_ );
        if ( fileInfo.exists() )
            lastModifiedDate_ = fileInfo.lastModified();
    }

    if ( file_ ) {
        // And we watch the file for updates
        fileChangedOnDisk_ = Unchanged;
        fileWatcher_.addFile( file_->fileName() );
    }

    emit loadingFinished( success );
}

//
// Implementation of virtual functions
//
qint64 LogData::doGetNbLine() const
{
    return nbLines_;
}

int LogData::doGetMaxLength() const
{
    return maxLength_;
}

QString LogData::doGetLineString( qint64 line ) const
{
    if ( line >= nbLines_ ) { return QString(); /* exception? */ }

    fileMutex_.lock();
    file_->open( QIODevice::ReadOnly );

    file_->seek( (line == 0) ? 0 : linePosition_[line-1] );
    QString string = QString( file_->readLine() );

    file_->close();
    fileMutex_.unlock();

    string.chop( 1 );

    return string;
}

QStringList LogData::doGetLines( qint64 first_line, int number ) const
{
    QStringList list;
    const qint64 last_line = first_line + number - 1;

    // LOG(logDEBUG) << "LogData::doGetLines first_line:" << first_line << " nb:" << number;

    if ( number == 0 ) {
        return QStringList();
    }

    if ( last_line >= nbLines_ ) {
        LOG(logWARNING) << "LogData::doGetLines Lines out of bound asked for";
        return QStringList(); /* exception? */
    }

    fileMutex_.lock();
    file_->open( QIODevice::ReadOnly );

    const qint64 first_byte = (first_line == 0) ? 0 : linePosition_[first_line-1];
    const qint64 last_byte  = linePosition_[last_line];
    // LOG(logDEBUG) << "LogData::doGetLines first_byte:" << first_byte << " last_byte:" << last_byte;
    file_->seek( first_byte );
    QByteArray blob = file_->read( last_byte - first_byte );

    file_->close();
    fileMutex_.unlock();

    qint64 beginning = 0;
    qint64 end = 0;
    for ( qint64 line = first_line; (line <= last_line); line++ ) {
        end = linePosition_[line] - first_byte;
        // LOG(logDEBUG) << "Getting line " << line << " beginning " << beginning << " end " << end;
        QByteArray this_line = blob.mid( beginning, end - beginning - 1 );
        // LOG(logDEBUG) << "Line is: " << QString( this_line ).toStdString();
        list.append( QString( this_line ) );
        beginning = end;
    }

    return list;
}
