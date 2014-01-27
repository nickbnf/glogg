/*
 * Copyright (C) 2009, 2010, 2013 Nicolas Bonnefon and other contributors
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

#include <cassert>

#include <QFileInfo>

#include "log.h"

#include "logdata.h"
#include "logfiltereddata.h"

// Implementation of the 'start' functions for each operation

void LogData::AttachOperation::doStart(
        LogDataWorkerThread& workerThread ) const
{
    LOG(logDEBUG) << "Attaching " << filename_.toStdString();
    workerThread.attachFile( filename_ );
    workerThread.indexAll();
}

void LogData::FullIndexOperation::doStart(
        LogDataWorkerThread& workerThread ) const
{
    LOG(logDEBUG) << "Reindexing (full)";
    workerThread.indexAll();
}

void LogData::PartialIndexOperation::doStart(
        LogDataWorkerThread& workerThread ) const
{
    LOG(logDEBUG) << "Reindexing (partial)";
    workerThread.indexAdditionalLines( filesize_ );
}


// Constructs an empty log file.
// It must be displayed without error.
LogData::LogData() : AbstractLogData(), fileWatcher_(), linePosition_(),
    fileMutex_(), dataMutex_(), workerThread_()
{
    // Start with an "empty" log
    file_         = nullptr;
    fileSize_     = 0;
    nbLines_      = 0;
    maxLength_    = 0;
    currentOperation_ = nullptr;
    nextOperation_    = nullptr;

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
    // FIXME
    // workerThread_.stop();
}

//
// Public functions
//

void LogData::attachFile( const QString& fileName )
{
    LOG(logDEBUG) << "LogData::attachFile " << fileName.toStdString();

    if ( file_ ) {
        // Remove the current file from the watch list
        fileWatcher_.removeFile( file_->fileName() );
    }

    workerThread_.interrupt();

    // If an attach operation is already in progress, the new one will
    // be delayed until the current one is finished (canceled)
    std::shared_ptr<const LogDataOperation> operation( new AttachOperation( fileName ) );
    enqueueOperation( std::move( operation ) );
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

void LogData::reload()
{
    workerThread_.interrupt();

    enqueueOperation( std::make_shared<FullIndexOperation>() );
}

//
// Private functions
//

// Add an operation to the queue and perform it immediately if
// there is none ongoing.
void LogData::enqueueOperation( std::shared_ptr<const LogDataOperation> new_operation )
{
    if ( currentOperation_ == nullptr )
    {
        // We do it immediately
        currentOperation_ =  new_operation;
        startOperation();
    }
    else
    {
        // An operation is in progress...
        // ... we schedule the attach op for later
        nextOperation_ = new_operation;
    }
}

// Performs the current operation asynchronously, a indexingFinished
// signal will be received when it's finished.
void LogData::startOperation()
{
    if ( currentOperation_ )
    {
        LOG(logDEBUG) << "startOperation found something to do.";

        // If it's a full indexing ...
        // ... we invalidate the non indexed data
        if ( currentOperation_->isFull() ) {
            fileSize_     = 0;
            nbLines_      = 0;
            maxLength_    = 0;
        }

        // And let the operation do its stuff
        currentOperation_->start( workerThread_ );
    }
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

    std::shared_ptr<LogDataOperation> newOperation;

    LOG(logDEBUG) << "current fileSize=" << fileSize_;
    LOG(logDEBUG) << "info file_->size()=" << info.size();
    if ( info.size() < fileSize_ ) {
        fileChangedOnDisk_ = Truncated;
        LOG(logINFO) << "File truncated";
        newOperation = std::make_shared<FullIndexOperation>();
    }
    else if ( fileChangedOnDisk_ != DataAdded ) {
        fileChangedOnDisk_ = DataAdded;
        LOG(logINFO) << "New data on disk";
        newOperation = std::make_shared<PartialIndexOperation>( fileSize_ );
    }

    if ( newOperation )
        enqueueOperation( newOperation );

    lastModifiedDate_ = info.lastModified();

    emit fileChanged( fileChangedOnDisk_ );
    // TODO: fileChangedOnDisk_, fileSize_
}

void LogData::indexingFinished( bool success )
{
    LOG(logDEBUG) << "Entering LogData::indexingFinished.";

    // We use the newly created file data or restore the old ones.
    // (Qt implicit copy makes this fast!)
    {
        QMutexLocker locker( &dataMutex_ );
        workerThread_.getIndexingData( &fileSize_, &maxLength_, &linePosition_ );
        nbLines_ = linePosition_.size();
    }

    LOG(logDEBUG) << "indexingFinished: " << success <<
        ", found " << nbLines_ << " lines.";

    if ( success ) {
        // Use the new filename if needed
        if ( !currentOperation_->getFilename().isNull() ) {
            QString newFileName = currentOperation_->getFilename();

            if ( file_ ) {
                QMutexLocker locker( &fileMutex_ );
                file_->setFileName( newFileName );
            }
            else {
                QMutexLocker locker( &fileMutex_ );
                file_.reset( new QFile( newFileName ) );
            }
        }

        // Update the modified date/time if the file exists
        lastModifiedDate_ = QDateTime();
        QFileInfo fileInfo( *file_ );
        if ( fileInfo.exists() )
            lastModifiedDate_ = fileInfo.lastModified();
    }

    if ( file_ ) {
        // And we watch the file for updates
        fileChangedOnDisk_ = Unchanged;
        fileWatcher_.addFile( file_->fileName() );
    }

    emit loadingFinished( success );

    // So now the operation is done, let's see if there is something
    // else to do, in which case, do it!
    assert( currentOperation_ );

    currentOperation_ = std::move( nextOperation_ );
    nextOperation_.reset();

    if ( currentOperation_ ) {
        LOG(logDEBUG) << "indexingFinished is performing the next operation";
        startOperation();
    }
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

int LogData::doGetLineLength( qint64 line ) const
{
    if ( line >= nbLines_ ) { return 0; /* exception? */ }

    int length = doGetExpandedLineString( line ).length();

    return length;
}

QString LogData::doGetLineString( qint64 line ) const
{
    if ( line >= nbLines_ ) { return QString(); /* exception? */ }

    dataMutex_.lock();
    fileMutex_.lock();
    file_->open( QIODevice::ReadOnly );

    file_->seek( (line == 0) ? 0 : linePosition_[line-1] );

    QString string = QString( file_->readLine() );

    file_->close();
    fileMutex_.unlock();
    dataMutex_.unlock();

    string.chop( 1 );

    return string;
}

QString LogData::doGetExpandedLineString( qint64 line ) const
{
    if ( line >= nbLines_ ) { return QString(); /* exception? */ }

    dataMutex_.lock();
    fileMutex_.lock();
    file_->open( QIODevice::ReadOnly );

    file_->seek( (line == 0) ? 0 : linePosition_[line-1] );

    QByteArray rawString = file_->readLine();

    file_->close();
    fileMutex_.unlock();
    dataMutex_.unlock();

    QString string = QString( untabify( rawString.constData() ) );
    string.chop( 1 );

    return string;
}

// Note this function is also called from the LogFilteredDataWorker thread, so
// data must be protected because they are changed in the main thread (by
// indexingFinished).
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

    dataMutex_.lock();

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

    dataMutex_.unlock();

    return list;
}

QStringList LogData::doGetExpandedLines( qint64 first_line, int number ) const
{
    QStringList list;
    const qint64 last_line = first_line + number - 1;

    if ( number == 0 ) {
        return QStringList();
    }

    if ( last_line >= nbLines_ ) {
        LOG(logWARNING) << "LogData::doGetExpandedLines Lines out of bound asked for";
        return QStringList(); /* exception? */
    }

    dataMutex_.lock();

    fileMutex_.lock();
    file_->open( QIODevice::ReadOnly );

    const qint64 first_byte = (first_line == 0) ? 0 : linePosition_[first_line-1];
    const qint64 last_byte  = linePosition_[last_line];
    // LOG(logDEBUG) << "LogData::doGetExpandedLines first_byte:" << first_byte << " last_byte:" << last_byte;
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
        list.append( untabify( this_line.constData() ) );
        beginning = end;
    }

    dataMutex_.unlock();

    return list;
}
