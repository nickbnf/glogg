/*
 * Copyright (C) 2009, 2010 Nicolas Bonnefon and other contributors
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

#include "logdata.h"
#include "logdataworkerthread.h"

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
    maxLength_     = qMax( maxLength_, length );
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
                emit indexingFinished( true );
            }
            else {
                emit indexingFinished( false );
            }

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

qint64 IndexOperation::doIndex( LinePositionArray& linePosition, int* maxLength,
        qint64 initialPosition )
{
    int max_length = *maxLength;
    qint64 pos = initialPosition; // Absolute position of the start of current line
    qint64 end = 0;               // Absolute position of the end of current line
    int additional_spaces = 0;    // Additional spaces due to tabs

    QFile file( fileName_ );
    if ( file.open( QIODevice::ReadOnly ) ) {
        // Count the number of lines and max length
        // (read big chunks to speed up reading from disk)
        file.seek( pos );
        while ( !file.atEnd() ) {
            if ( *interruptRequest_ )   // a bool is always read/written atomically isn't it?
                break;

            // Read a chunk of 5MB
            const qint64 block_beginning = file.pos();
            const QByteArray block = file.read( sizeChunk );

            // Count the number of lines in each chunk
            qint64 pos_within_block = 0;
            while ( pos_within_block != -1 ) {
                pos_within_block = qMax( pos - block_beginning, 0LL);
                // Looking for the next \n, expanding tabs in the process
                do {
                    if ( pos_within_block < block.length() ) {
                        const char c = block.at(pos_within_block);
                        if ( c == '\n' )
                            break;
                        else if ( c == '\t' )
                            additional_spaces += AbstractLogData::tabStop -
                                ( ( ( block_beginning - pos ) + pos_within_block
                                    + additional_spaces ) % AbstractLogData::tabStop ) - 1;

                        pos_within_block++;
                    }
                    else {
                        pos_within_block = -1;
                    }
                } while ( pos_within_block != -1 );

                // When a end of line has been found...
                if ( pos_within_block != -1 ) {
                    end = pos_within_block + block_beginning;
                    const int length = end-pos + additional_spaces;
                    if ( length > max_length )
                        max_length = length;
                    pos = end + 1;
                    additional_spaces = 0;
                    linePosition.append( pos );
                }
            }

            // Update the caller for progress indication
            int progress = ( file.size() > 0 ) ? pos*100 / file.size() : 100;
            emit indexingProgressed( progress );
        }

        // Check if there is a non LF terminated line at the end of the file
        if ( file.size() > pos ) {
            LOG( logWARNING ) <<
                "Non LF terminated file, adding a fake end of line";
            linePosition.append( file.size() + 1 );
            linePosition.setFakeFinalLF();
        }
    }
    else {
        // TODO: Check that the file is seekable?
        // If the file cannot be open, we do as if it was empty
        LOG(logWARNING) << "Cannot open file " << fileName_.toStdString();

        emit indexingProgressed( 100 );
    }

    *maxLength = max_length;

    return file.size();
}

// Called in the worker thread's context
// Should not use any shared variable
bool FullIndexOperation::start( IndexingData& sharedData )
{
    LOG(logDEBUG) << "FullIndexOperation::start(), file "
        << fileName_.toStdString();

    LOG(logDEBUG) << "FullIndexOperation: Starting the count...";
    int maxLength = 0;
    LinePositionArray linePosition = LinePositionArray();

    emit indexingProgressed( 0 );

    qint64 size = doIndex( linePosition, &maxLength, 0 );

    if ( *interruptRequest_ == false )
    {
        // Commit the results to the shared data (atomically)
        sharedData.setAll( size, maxLength, linePosition );
    }

    LOG(logDEBUG) << "FullIndexOperation: ... finished counting."
        "interrupt = " << *interruptRequest_;

    return ( *interruptRequest_ ? false : true );
}

bool PartialIndexOperation::start( IndexingData& sharedData )
{
    LOG(logDEBUG) << "PartialIndexOperation::start(), file "
        << fileName_.toStdString();

    LOG(logDEBUG) << "PartialIndexOperation: Starting the count at "
        << initialPosition_ << " ...";
    int maxLength = 0;
    LinePositionArray linePosition = LinePositionArray();

    emit indexingProgressed( 0 );

    qint64 size = doIndex( linePosition, &maxLength, initialPosition_ );

    if ( *interruptRequest_ == false )
    {
        // Commit the results to the shared data (atomically)
        sharedData.addAll( size - initialPosition_, maxLength, linePosition );
    }

    LOG(logDEBUG) << "PartialIndexOperation: ... finished counting.";

    return ( *interruptRequest_ ? false : true );
}
