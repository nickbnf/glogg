/*
 * Copyright (C) 2009, 2010, 2014, 2015 Nicolas Bonnefon and other contributors
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

#include <atomic>
#include <QFile>

#include "log.h"

#include "logdata.h"
#include "logdataworkerthread.h"

// Size of the chunk to read (5 MiB)
const int IndexOperation::sizeChunk = 5*1024*1024;

qint64 IndexingData::getSize() const
{
    QMutexLocker locker( &dataMutex_ );

    return indexedSize_;
}

int IndexingData::getMaxLength() const
{
    QMutexLocker locker( &dataMutex_ );

    return maxLength_;
}

LineNumber IndexingData::getNbLines() const
{
    QMutexLocker locker( &dataMutex_ );

    return linePosition_.size();
}

qint64 IndexingData::getPosForLine( LineNumber line ) const
{
    QMutexLocker locker( &dataMutex_ );

    return linePosition_.at( line );
}

EncodingSpeculator::Encoding IndexingData::getEncodingGuess() const
{
    QMutexLocker locker( &dataMutex_ );

    return encoding_;
}

void IndexingData::addAll( qint64 size, int length,
        const FastLinePositionArray& linePosition,
        EncodingSpeculator::Encoding encoding )

{
    QMutexLocker locker( &dataMutex_ );

    indexedSize_  += size;
    maxLength_     = qMax( maxLength_, length );
    linePosition_.append_list( linePosition );

    encoding_      = encoding;
}

void IndexingData::clear()
{
    maxLength_   = 0;
    indexedSize_ = 0;
    linePosition_ = LinePositionArray();
    encoding_    = EncodingSpeculator::Encoding::ASCII7;
}

LogDataWorkerThread::LogDataWorkerThread( IndexingData& indexing_data )
    : indexing_data_( indexing_data )
{
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

    interruptRequested_.store( false , std::memory_order_relaxed );
    operationRequested_ = new FullIndexOperation( fileName_,
            indexing_data_, interruptRequested_, encodingSpeculator_ );
    operationRequestedCond_.wakeAll();
}

void LogDataWorkerThread::indexAdditionalLines()
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "AddLines requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != NULL) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_.store( false, std::memory_order_relaxed );
    operationRequested_ = new PartialIndexOperation( fileName_,
            indexing_data_, interruptRequested_, encodingSpeculator_ );
    operationRequestedCond_.wakeAll();
}

void LogDataWorkerThread::interrupt()
{
    LOG(logDEBUG) << "Load interrupt requested";

    interruptRequested_.store( true, std::memory_order_relaxed );
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
            try {
                if ( operationRequested_->start() ) {
                    LOG(logDEBUG) << "... finished copy in workerThread.";
                    emit indexingFinished( LoadingStatus::Successful );
                }
                else {
                    emit indexingFinished( LoadingStatus::Interrupted );
                }
            }
            catch ( std::bad_alloc& ba ) {
                LOG(logERROR) << "Out of memory whilst indexing!";
                emit indexingFinished( LoadingStatus::NoMemory );
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

IndexOperation::IndexOperation( const QString& fileName,
        IndexingData& indexingData, std::atomic_bool& interruptRequest,
        EncodingSpeculator& encodingSpeculator )
    : fileName_( fileName )
    , interruptRequest_( interruptRequest )
    , indexing_data_( indexingData )
    , encoding_speculator_( encodingSpeculator )
{
}

void IndexOperation::doIndex( qint64 initialPosition )
{
    qint64 pos = initialPosition; // Absolute position of the start of current line
    qint64 end = 0;               // Absolute position of the end of current line
    int additional_spaces = 0;    // Additional spaces due to tabs

    QFile file( fileName_ );
    if ( file.open( QIODevice::ReadOnly ) ) {
        // Count the number of lines and max length
        // (read big chunks to speed up reading from disk)
        file.seek( pos );
        while ( !file.atEnd() ) {
            FastLinePositionArray line_positions;
            int max_length = 0;

            if ( interruptRequest_.load( std::memory_order_relaxed ) )
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
                        encoding_speculator_.inject_byte( c );
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
                    line_positions.append( pos );
                }
            }

            // Update the shared data
            indexing_data_.addAll( block.length(), max_length, line_positions,
                   encoding_speculator_.guess() );

            // Update the caller for progress indication
            int progress = ( file.size() > 0 ) ? pos*100 / file.size() : 100;
            emit indexingProgressed( progress );
        }

        // Check if there is a non LF terminated line at the end of the file
        qint64 file_size = file.size();
        if ( !interruptRequest_.load( std::memory_order_relaxed ) && file_size > pos ) {
            LOG( logWARNING ) <<
                "Non LF terminated file, adding a fake end of line";

            FastLinePositionArray line_position;
            line_position.append( file_size + 1 );
            line_position.setFakeFinalLF();

            indexing_data_.addAll( 0, 0, line_position, encoding_speculator_.guess() );
        }
    }
    else {
        // TODO: Check that the file is seekable?
        // If the file cannot be open, we do as if it was empty
        LOG(logWARNING) << "Cannot open file " << fileName_.toStdString();

        emit indexingProgressed( 100 );
    }
}

// Called in the worker thread's context
bool FullIndexOperation::start()
{
    LOG(logDEBUG) << "FullIndexOperation::start(), file "
        << fileName_.toStdString();

    LOG(logDEBUG) << "FullIndexOperation: Starting the count...";

    emit indexingProgressed( 0 );

    // First empty the index
    indexing_data_.clear();

    doIndex( 0 );

    bool interruptRequest = interruptRequest_.load( std::memory_order_relaxed );

    LOG(logDEBUG) << "FullIndexOperation: ... finished counting."
        "interrupt = " << interruptRequest;

    return !interruptRequest;
}

bool PartialIndexOperation::start()
{
    LOG(logDEBUG) << "PartialIndexOperation::start(), file "
        << fileName_.toStdString();

    qint64 initial_position = indexing_data_.getSize();

    LOG(logDEBUG) << "PartialIndexOperation: Starting the count at "
        << initial_position << " ...";

    emit indexingProgressed( 0 );

    doIndex( initial_position );

    bool interruptRequest = interruptRequest_.load( std::memory_order_relaxed );

    LOG(logDEBUG) << "PartialIndexOperation: ... finished counting.";

    return !interruptRequest;
}
