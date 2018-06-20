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

#include <QFile>
#include <QTextStream>

#include <stlab/concurrency/channel.hpp>
#include <stlab/concurrency/default_executor.hpp>

#include "log.h"

#include "logdata.h"
#include "logdataworkerthread.h"

#include "persistentinfo.h"
#include "configuration.h"

#include <chrono>

qint64 IndexingData::getSize() const
{
    QMutexLocker locker( &dataMutex_ );

    return indexedSize_;
}

LineLength IndexingData::getMaxLength() const
{
    QMutexLocker locker( &dataMutex_ );

    return maxLength_;
}

LinesCount IndexingData::getNbLines() const
{
    QMutexLocker locker( &dataMutex_ );

    return LinesCount( linePosition_.size() );
}

LineOffset IndexingData::getPosForLine( LineNumber line ) const
{
    QMutexLocker locker( &dataMutex_ );

    return linePosition_.at( line.get() );
}

QTextCodec *IndexingData::getEncodingGuess() const
{
    QMutexLocker locker( &dataMutex_ );
    return encodingGuess_;
}

void IndexingData::forceEncoding(QTextCodec *codec)
{
     QMutexLocker locker( &dataMutex_ );
     encodingForced_ = codec;
}

QTextCodec* IndexingData::getForcedEncoding() const
{
     QMutexLocker locker( &dataMutex_ );
     return encodingForced_;
}

void IndexingData::addAll(qint64 size, LineLength length,
        const FastLinePositionArray& linePosition,
        QTextCodec *encoding )

{
    QMutexLocker locker( &dataMutex_ );

    indexedSize_  += size;
    maxLength_     = qMax( maxLength_, length );
    linePosition_.append_list( linePosition );

    encodingGuess_      = encoding;
}

void IndexingData::clear()
{
	QMutexLocker locker(&dataMutex_);

    maxLength_   = 0_length;
    indexedSize_ = 0;
    linePosition_ = LinePositionArray();
    encodingGuess_    = QTextCodec::codecForLocale();
    encodingForced_ = nullptr;
}

LogDataWorkerThread::LogDataWorkerThread( IndexingData* indexing_data )
    : QThread(), mutex_(), operationRequestedCond_(),
    nothingToDoCond_(), fileName_(), indexing_data_( indexing_data )
{
    operationRequested_ = nullptr;
}

LogDataWorkerThread::~LogDataWorkerThread()
{
    interruptRequested_.set();
    {
        QMutexLocker locker( &mutex_ );
        terminate_.set();
        operationRequestedCond_.wakeAll();
    }
    wait();
}

void LogDataWorkerThread::attachFile( const QString& fileName )
{
    QMutexLocker locker( &mutex_ );  // to protect fileName_

    fileName_ = fileName;
}

void LogDataWorkerThread::indexAll(QTextCodec* forcedEncoding)
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "FullIndex requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != nullptr) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_.clear();
    operationRequested_ = new FullIndexOperation( fileName_,
            indexing_data_, &interruptRequested_, forcedEncoding );
    operationRequestedCond_.wakeAll();
}

void LogDataWorkerThread::indexAdditionalLines()
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "AddLines requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != nullptr) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_.clear();
    operationRequested_ = new PartialIndexOperation( fileName_,
            indexing_data_, &interruptRequested_ );

    operationRequestedCond_.wakeAll();
}

void LogDataWorkerThread::interrupt()
{
    LOG(logDEBUG) << "Load interrupt requested";
    interruptRequested_.set();
}

// This is the thread's main loop
void LogDataWorkerThread::run()
{
    QMutexLocker locker( &mutex_ );

    forever {
        while ( !terminate_ && (operationRequested_ == nullptr) )
            operationRequestedCond_.wait( &mutex_ );
        LOG(logDEBUG) << "Worker thread signaled";

        // Look at what needs to be done
        if ( terminate_ )
            return;      // We must die

        if ( operationRequested_ ) {
            connect( operationRequested_, &IndexOperation::indexingProgressed,
                    this, &LogDataWorkerThread::indexingProgressed );

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
            catch ( std::bad_alloc& ) {
                LOG(logERROR) << "Out of memory whilst indexing!";
                emit indexingFinished( LoadingStatus::NoMemory );
            }

            delete operationRequested_;
            operationRequested_ = nullptr;
            nothingToDoCond_.wakeAll();
        }
    }
}

//
// Operations implementation
//

IndexOperation::IndexOperation( const QString& fileName,
        IndexingData* indexingData, AtomicFlag* interruptRequest)
    : fileName_( fileName )
{
    interruptRequest_ = interruptRequest;
    indexing_data_ = indexingData;
}

PartialIndexOperation::PartialIndexOperation( const QString& fileName,
        IndexingData* indexingData, AtomicFlag* interruptRequest )
    : IndexOperation( fileName, indexingData, interruptRequest )
{
}

FastLinePositionArray IndexOperation::parseDataBlock( int block_beginning,
                                                     const QByteArray &block,
                                                     IndexingState &state ) const
{
    state.max_length = 0;
    FastLinePositionArray line_positions;

    int pos_within_block = 0;
    while ( pos_within_block != -1 ) {
        pos_within_block = qMax( static_cast<int>( state.pos - block_beginning ), 0 );

        // Looking for the next \n, expanding tabs in the process
        do {
            if ( pos_within_block < block.length() ) {
                const char c =  block.at( pos_within_block + state.encodingParams.lineFeedIndex );

                if ( c == '\n')
                    break;
                else if ( c == '\t')
                    state.additional_spaces += AbstractLogData::tabStop -
                        ( static_cast<int>( ( block_beginning - state.pos ) + pos_within_block
                            + state.additional_spaces ) % AbstractLogData::tabStop ) - 1;

                pos_within_block += state.encodingParams.lineFeedWidth;
            }
            else {
                pos_within_block = -1;
            }
        } while ( pos_within_block != -1 );

        // When a end of line has been found...
        if ( pos_within_block != -1 ) {
            state.end = pos_within_block + block_beginning;
            const auto length = static_cast<LineLength::UnderlyingType>( state.end - state.pos + state.additional_spaces );
            if ( length > state.max_length )
                state.max_length = length;

            state.pos = state.end + state.encodingParams.lineFeedWidth;
            state.additional_spaces = 0;
            line_positions.append( LineOffset( state.pos ) );
        }
    }

    return line_positions;

}

void IndexOperation::guessEncoding( const QByteArray& block, IndexingState& state ) const
{
    if ( !state.fileTextCodec ) {
        state.fileTextCodec = indexing_data_->getForcedEncoding();
        if ( !state.fileTextCodec ) {
            state.fileTextCodec = EncodingDetector::getInstance().detectEncoding( block );
        }

        state.encodingParams = EncodingParameters( state.fileTextCodec );
        LOG(logINFO) << "Encoding " << state.fileTextCodec->name().toStdString()
                      << ", Char width " << state.encodingParams.lineFeedWidth;
    }

    if ( !state.encodingGuess ) {
        state.encodingGuess = EncodingDetector::getInstance().detectEncoding( block );
        LOG(logINFO) << "Encoding guess " << state.encodingGuess->name().toStdString();
    }
}

auto IndexOperation::setupIndexingProcess( IndexingState &indexingState )
{
    using BlockData = std::pair<int,QByteArray>;
    stlab::sender<BlockData> blockSender;
    stlab::receiver<BlockData> indexer;

    std::tie( blockSender, indexer ) = stlab::channel<BlockData>( stlab::default_executor );

    auto process = indexer | [this, &indexingState]( const BlockData& blockData )
    {
        const auto block_beginning = blockData.first;
        const auto& block = blockData.second;

        LOG(logDEBUG) << "Indexing block " << block_beginning;

        if ( block.isEmpty() ) {
            QMutexLocker lock( &indexingState.indexingMutex );
            indexingState.indexedSize = indexingState.file_size;
            indexingState.indexingDone.wakeAll();
            return;
        }

        guessEncoding( block, indexingState );

        auto line_positions = parseDataBlock( block_beginning, block, indexingState );
        indexing_data_->addAll( block.length(),
                               LineLength( indexingState.max_length ),
                               line_positions, indexingState.encodingGuess );

        // Update the caller for progress indication
        const auto progress = static_cast<int>( ( indexingState.file_size > 0 )
                                                ? indexingState.pos*100 / indexingState.file_size
                                                : 100 );
        emit indexingProgressed( progress );

        {
            QMutexLocker lock( &indexingState.indexingMutex );
            indexingState.indexedSize = block_beginning + block.size();
            indexingState.blockDone.wakeAll();
        }
    };

    indexer.set_ready();

    return std::make_tuple(std::move(blockSender), std::move(indexer), std::move(process));
}

void IndexOperation::doIndex(LineOffset initialPosition )
{
    static std::shared_ptr<Configuration> config =
        Persistent<Configuration>( "settings" );

    const uint32_t sizeChunk = 1024 * 1024;
    const auto prefetchBufferSize = config->indexReadBufferSizeMb() * sizeChunk;

    QFile file( fileName_ );

    if ( file.isOpen() || file.open( QIODevice::ReadOnly ) ) {

        IndexingState state;
        state.pos = initialPosition.get();
        state.end = 0ll;
        state.additional_spaces = 0;
        state.max_length = 0;
        state.encodingGuess = nullptr;
        state.fileTextCodec = nullptr;
        state.indexedSize = 0;
        state.file_size = file.size();

        auto process = setupIndexingProcess(state);
        auto blockSender = std::get<0>(process);

        using namespace std::chrono;
        high_resolution_clock::time_point t1 = high_resolution_clock::now();

        file.seek( state.pos );
        while ( !file.atEnd() ) {

            if ( *interruptRequest_ )
                break;

            const auto block_beginning = file.pos();
            const auto block = file.read( sizeChunk );

            bool shouldWait = false;
            do
            {
                QMutexLocker lock( &state.indexingMutex );
                shouldWait = block_beginning - state.indexedSize > prefetchBufferSize;
                if ( shouldWait ) {
                    state.blockDone.wait( &state.indexingMutex );
                }
            } while( shouldWait );

            LOG(logDEBUG) << "Sending block " << block_beginning;
            blockSender(std::make_pair(block_beginning, block));
        }

        blockSender(std::make_pair(state.file_size, QByteArray{}));
        blockSender.close();

        {
            QMutexLocker lock( &state.indexingMutex );
            if (state.indexedSize < state.file_size ) {
                state.indexingDone.wait( &state.indexingMutex );
            }
        }

        // Check if there is a non LF terminated line at the end of the file
        if ( !*interruptRequest_ && state.file_size > state.pos ) {
            LOG( logWARNING ) <<
                "Non LF terminated file, adding a fake end of line";

            FastLinePositionArray line_position;
            line_position.append( LineOffset( state.file_size + 1 ) );
            line_position.setFakeFinalLF();

            indexing_data_->addAll( 0, 0_length, line_position, state.encodingGuess );
        }

        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>( t2 - t1 ).count();

        LOG( logINFO ) << "Indexing done, took " << duration << " ms";
        LOG( logINFO ) << "Indexing perf " << (1000.f * state.file_size / duration) / (1024*1024) << " MiB/s";
    }
    else {
        // TODO: Check that the file is seekable?
        // If the file cannot be open, we do as if it was empty
        LOG(logWARNING) << "Cannot open file " << fileName_.toStdString();

        indexing_data_->clear();

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
    indexing_data_->clear();
    indexing_data_->forceEncoding(forcedEncoding_);

    doIndex( 0_offset );

    LOG(logDEBUG) << "FullIndexOperation: ... finished counting."
        "interrupt = " << static_cast<bool>(*interruptRequest_);

    return ( *interruptRequest_ ? false : true );
}

bool PartialIndexOperation::start()
{
    LOG(logDEBUG) << "PartialIndexOperation::start(), file "
        << fileName_.toStdString();

    const auto initial_position = LineOffset(indexing_data_->getSize());

    LOG(logDEBUG) << "PartialIndexOperation: Starting the count at "
        << initial_position << " ...";

    emit indexingProgressed( 0 );

    doIndex( initial_position );

    LOG(logDEBUG) << "PartialIndexOperation: ... finished counting.";

    return ( *interruptRequest_ ? false : true );
}
