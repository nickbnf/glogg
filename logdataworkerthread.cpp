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

#include <QFile>

#include "log.h"

#include "logdataworkerthread.h"
#include "common.h"

// Size of the chunk to read (5 MiB)
const int IndexOperation::sizeChunk = 5*1024*1024;

void IndexingData::getAll( qint64* size, int* length,
        LinePositionArray* linePosition )
{
    QMutexLocker locker( &dataMutex_ );

    *size         = indexedSize_;
    *length       = maxLength_;
    *linePosition = linePosition_;
}

void IndexingData::setAll( qint64 size, int length,
        const LinePositionArray& linePosition )
{
    QMutexLocker locker( &dataMutex_ );

    indexedSize_  = size;
    maxLength_    = length;
    linePosition_ = linePosition;
}

void IndexingData::addAll( qint64 size, int length,
        const LinePositionArray& linePosition )
{
    QMutexLocker locker( &dataMutex_ );

    indexedSize_  += size;
    maxLength_     = max2( maxLength_, length );
    linePosition_ += linePosition;
}

LogDataWorkerThread::LogDataWorkerThread()
    : QThread(), mutex_(), operationRequestedCond_(),
    nothingToDoCond_(), fileName_(), indexingData_()
{
    terminate_          = false;
    interruptRequested_ = false;
    operationRequested_ = NULL;
}

LogDataWorkerThread::~LogDataWorkerThread()
{
    {
        QMutexLocker locker( &mutex_ );
        terminate_ = true;
        operationRequestedCond_.wakeAll();
    }
    wait();
}

void LogDataWorkerThread::attachFile( const QString& fileName )
{
    QMutexLocker locker( &mutex_ );  // to protect fileName_

    fileName_ = fileName;
}

void LogDataWorkerThread::indexAll()
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "FullIndex requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != NULL) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_ = false;
    operationRequested_ = new FullIndexOperation( fileName_, &interruptRequested_ );
    operationRequestedCond_.wakeAll();
}

void LogDataWorkerThread::indexAdditionalLines( qint64 position )
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "AddLines requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != NULL) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_ = false;
    operationRequested_ = new PartialIndexOperation( fileName_, &interruptRequested_, position );
    operationRequestedCond_.wakeAll();
}

void LogDataWorkerThread::interrupt()
{
    LOG(logDEBUG) << "Load interrupt requested";

    // No mutex here, setting a bool is probably atomic!
    interruptRequested_ = true;
}

// This will do an atomic copy of the object
// (hopefully fast as we use Qt containers)
void LogDataWorkerThread::getIndexingData(
        qint64* indexedSize, int* maxLength, LinePositionArray* linePosition )
{
    indexingData_.getAll( indexedSize, maxLength, linePosition );
}

// This is the thread's main loop
void LogDataWorkerThread::run()
{
    QMutexLocker locker( &mutex_ );

    forever {
        while ( (terminate_ == false) && (operationRequested_ == NULL) )
            operationRequestedCond_.wait( &mutex_ );
        LOG(logDEBUG) << "Worker thread signaled";

        // Look at what needs to be done
        if ( terminate_ )
            return;      // We must die

        if ( operationRequested_ ) {
            connect( operationRequested_, SIGNAL( indexingProgressed( int ) ),
                    this, SIGNAL( indexingProgressed( int ) ) );

            // Run the operation
            if ( operationRequested_->start( indexingData_ ) ) {
                LOG(logDEBUG) << "... finished copy in workerThread.";
            }

            emit indexingFinished();
            delete operationRequested_;
            operationRequested_ = NULL;
            nothingToDoCond_.wakeAll();
        }
    }
}

//
// Operations implementation
//

IndexOperation::IndexOperation( QString& fileName, bool* interruptRequest )
    : fileName_( fileName )
{
    interruptRequest_ = interruptRequest;
}

PartialIndexOperation::PartialIndexOperation( QString& fileName,
        bool* interruptRequest, qint64 position )
    : IndexOperation( fileName, interruptRequest )
{
    initialPosition_ = position;
}

// Called in the worker thread's context
// Should not use any shared variable
bool FullIndexOperation::start( IndexingData& sharedData )
{
    LOG(logDEBUG) << "FullIndexOperation::start(), file "
        << fileName_.toStdString();

    // Count the number of lines and max length
    // (read big chunks to speed up reading from disk)
    LOG(logDEBUG) << "FullIndexOperation: Starting the count...";
    int maxLength = 0;
    LinePositionArray linePosition = LinePositionArray();
    qint64 end = 0, pos = 0;

    QFile file( fileName_ );
    if ( file.open( QIODevice::ReadOnly ) ) {
        while ( !file.atEnd() ) {
            if ( *interruptRequest_ )   // a bool is always read/written atomically isn't it?
                break;

            // Read a chunk of 5MB
            const qint64 block_beginning = file.pos();
            const QByteArray block = file.read( sizeChunk );

            // Count the number of lines in each chunk
            qint64 next_lf = 0;
            while ( next_lf != -1 ) {
                const qint64 pos_within_block = max2( pos - block_beginning, 0LL);
                next_lf = block.indexOf( "\n", pos_within_block );
                if ( next_lf != -1 ) {
                    end = next_lf + block_beginning;
                    const int length = end-pos;
                    if ( length > maxLength )
                        maxLength = length;
                    pos = end + 1;
                    linePosition.append( pos );
                }
            }

            // Update the caller for progress indication
            int progress = ( file.size() > 0 ) ? pos*100 / file.size() : 100;
            emit indexingProgressed( progress );
        }
    }
    else {
        // TODO: Check that the file is seekable?
        // If the file cannot be open, we do as if it was empty
        LOG(logWARNING) << "Cannot open file " << fileName_.toStdString();

        emit indexingProgressed( 100 );
    }

    if ( *interruptRequest_ == false )
    {
        // Commit the results to the shared data (atomically)
        sharedData.setAll( pos, maxLength, linePosition );
    }

    LOG(logDEBUG) << "FullIndexOperation: ... finished counting."
        "interrupt = " << *interruptRequest_;

    return ( *interruptRequest_ ? false : true );
}

bool PartialIndexOperation::start( IndexingData& sharedData )
{
    LOG(logDEBUG) << "PartialIndexOperation::start(), file "
        << fileName_.toStdString();

    QFile file( fileName_ );
    if ( !file.open( QIODevice::ReadOnly ) )
    {
        // TODO: Check that the file is seekable?
        LOG(logERROR) << "Cannot open file " << fileName_.toStdString();
        return false;
    }

    // Count the number of lines and max length
    // (read big chunks to speed up reading from disk)
    LOG(logDEBUG) << "PartialIndexOperation: Starting the count at "
        << initialPosition_ << " ...";
    int maxLength = 0;
    LinePositionArray linePosition = LinePositionArray();
    qint64 end = 0, pos = initialPosition_;
    file.seek( pos );
    while ( !file.atEnd() ) {
        if ( *interruptRequest_ )   // a bool is always read/written atomically isn't it?
            break;

        // Read a chunk of 5MB
        const qint64 block_beginning = file.pos();
        const QByteArray block = file.read( sizeChunk );

        // Count the number of lines in each chunk
        qint64 next_lf = 0;
        while ( next_lf != -1 ) {
            const qint64 pos_within_block = max2( pos - block_beginning, 0LL);
            next_lf = block.indexOf( "\n", pos_within_block );
            if ( next_lf != -1 ) {
                end = next_lf + block_beginning;
                const int length = end-pos;
                if ( length > maxLength )
                    maxLength = length;
                pos = end + 1;
                linePosition.append( pos );
            }
        }
    }

    if ( *interruptRequest_ == false )
    {
        // Commit the results to the shared data (atomically)
        sharedData.addAll( pos - initialPosition_, maxLength, linePosition );
    }

    LOG(logDEBUG) << "PartialIndexOperation: ... finished counting.";

    return ( *interruptRequest_ ? false : true );
}
