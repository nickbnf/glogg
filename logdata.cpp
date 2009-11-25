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
LogData::LogData() : AbstractLogData(), fileWatcher( this ), linePosition_(), workerThread_()
{
    // Start with an "empty" log
    file_         = NULL;
    fileSize_     = 0;
    nbLines_      = 0;
    maxLength_    = 0;

    indexingInProgress_ = false;

    // Initialise the file watcher
    connect( &fileWatcher, SIGNAL( fileChanged( const QString& ) ),
            this, SLOT( fileChangedOnDisk() ) );
    // Forward the update signal
    connect( &workerThread_, SIGNAL( indexingProgressed( int ) ),
            this, SIGNAL( loadingProgressed( int ) ) );
    connect( &workerThread_, SIGNAL( indexingFinished() ),
            this, SLOT( indexingFinished() ) );

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

bool LogData::attachFile( const QString& fileName )
{
    if ( file_ ) {
        // Remove the current file from the watch list
        fileWatcher.removePath( file_->fileName() );
        // And use the new filename
        file_->close();
        file_->setFileName( fileName );
    }
    else {
        file_ = new QFile( fileName );
    }

    // Open the new file
    if ( !file_->open( QIODevice::ReadOnly ) )
    {
        // TODO: Check that the file is seekable?
        LOG(logERROR) << "Cannot open file " << fileName.toStdString();
        return false;
    }

    // Now the file is open, so we are committed to loading it
    // We invalidate the non indexed data
    fileSize_     = 0;
    nbLines_      = 0;
    maxLength_    = 0;

    // And instructs the worker thread to index the whole file asynchronously
    LOG(logDEBUG) << "Attaching " << fileName.toStdString();
    indexingInProgress_ = true;
    workerThread_.attachFile( fileName );
    workerThread_.indexAll();

    // The client might now use the new file (even if it is empty for now)!
    return true;
}

void LogData::interruptLoading()
{
    if ( indexingInProgress_ )
        workerThread_.interrupt();
}

qint64 LogData::getFileSize() const
{
    return fileSize_;
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
    if ( fileChangedOnDisk_ != TRUNCATED ) {
        if ( file_->size() < fileSize_ ) {
            fileChangedOnDisk_ = TRUNCATED;
            LOG(logINFO) << "File truncated";
            workerThread_.indexAll();
        }
        else if ( fileChangedOnDisk_ != DATA_ADDED ) {
            fileChangedOnDisk_ = DATA_ADDED;
            LOG(logINFO) << "New data on disk";
            workerThread_.indexAdditionalLines( fileSize_ );
        }
    }
}

void LogData::indexingFinished()
{
    LOG(logDEBUG) << "indexingFinished: now copying in LogData.";

    indexingInProgress_ = false;

    // We use the newly created file data
    // (Qt implicit copy makes this fast!)
    workerThread_.getIndexingData( &fileSize_, &maxLength_, &linePosition_ );
    nbLines_      = linePosition_.size();

    LOG(logDEBUG) << "indexingFinished: found " << nbLines_ << " lines.";

    // And we watch the file for updates
    fileChangedOnDisk_ = UNCHANGED;
    fileWatcher.addPath( file_->fileName() );

    emit loadingFinished();
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
    if ( line >= nbLines_ ) { return QString(); /* exception? */ }

    file_->seek( linePosition_[line] );
    QString string = QString( file_->readLine() );
    string.chop( 1 );

    return string;
}
