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

/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QFile>
#include <QtConcurrent>

#include "log.h"

#include "logdata.h"
#include "logdataworker.h"

#include "configuration.h"
#include "readablesize.h"

#include <chrono>
#include <cmath>

#include <tbb/flow_graph.h>

namespace {

constexpr int IndexingBlockSize = 1 * 1024 * 1024;

template <class... Fs>
struct overload;

template <class F0, class... Frest>
struct overload<F0, Frest...> : F0, overload<Frest...> {
    overload( F0 f0, Frest... rest )
        : F0( f0 )
        , overload<Frest...>( rest... )
    {
    }

    using F0::operator();
    using overload<Frest...>::operator();
};

template <class F0>
struct overload<F0> : F0 {
    explicit overload( F0 f0 )
        : F0( f0 )
    {
    }

    using F0::operator();
};

template <class... Fs>
auto make_visitor( Fs... fs )
{
    return overload<Fs...>( fs... );
}

} // namespace

qint64 IndexingData::getIndexedSize() const
{
    return hash_.size;
}

IndexedHash IndexingData::getHash() const
{
    return hash_;
}

LineLength IndexingData::getMaxLength() const
{
    return maxLength_;
}

LinesCount IndexingData::getNbLines() const
{
    return LinesCount( linePosition_.size() );
}

LineOffset IndexingData::getPosForLine( LineNumber line ) const
{
    return linePosition_.at( line.get() );
}

QTextCodec* IndexingData::getEncodingGuess() const
{
    return encodingGuess_;
}

void IndexingData::setEncodingGuess( QTextCodec* codec )
{
    encodingGuess_ = codec;
}

void IndexingData::forceEncoding( QTextCodec* codec )
{
    encodingForced_ = codec;
}

QTextCodec* IndexingData::getForcedEncoding() const
{
    return encodingForced_;
}

void IndexingData::addAll( const QByteArray& block, LineLength length,
                           const FastLinePositionArray& linePosition, QTextCodec* encoding )

{
    maxLength_ = qMax( maxLength_, length );
    linePosition_.append_list( linePosition );

    if ( !block.isEmpty() ) {
        hash_.size += block.size();

        const auto& config = Configuration::get();
        if ( !config.fastModificationDetection() ) {
            hashBuilder_.addData( block.data(), static_cast<size_t>( block.size() ) );
            hash_.fullDigest = hashBuilder_.digest();
        }
    }

    encodingGuess_ = encoding;
}

void IndexingData::clear()
{
    maxLength_ = 0_length;
    hash_ = {};
    hashBuilder_.reset();
    linePosition_ = LinePositionArray();
    encodingGuess_ = nullptr;
    encodingForced_ = nullptr;
}

size_t IndexingData::allocatedSize() const
{
    return linePosition_.allocatedSize();
}

LogDataWorker::LogDataWorker( IndexingData& indexing_data )
    : indexing_data_( indexing_data )
{
}

LogDataWorker::~LogDataWorker()
{
    interruptRequest_.set();
    ScopedLock locker( &mutex_ );
    operationFuture_.waitForFinished();
}

void LogDataWorker::attachFile( const QString& fileName )
{
    ScopedLock locker( &mutex_ ); // to protect fileName_
    fileName_ = fileName;
}

void LogDataWorker::indexAll( QTextCodec* forcedEncoding )
{
    ScopedLock locker( &mutex_ );
    LOG_DEBUG << "FullIndex requested";

    operationFuture_.waitForFinished();
    interruptRequest_.clear();

    operationFuture_ = QtConcurrent::run( [this, forcedEncoding, fileName = fileName_] {
        auto operationRequested = std::make_unique<FullIndexOperation>(
            fileName, indexing_data_, interruptRequest_, forcedEncoding );
        return connectSignalsAndRun( operationRequested.get() );
    } );
}

void LogDataWorker::indexAdditionalLines()
{
    ScopedLock locker( &mutex_ );
    LOG_DEBUG << "AddLines requested";

    operationFuture_.waitForFinished();
    interruptRequest_.clear();

    operationFuture_ = QtConcurrent::run( [this, fileName = fileName_] {
        auto operationRequested = std::make_unique<PartialIndexOperation>( fileName, indexing_data_,
                                                                           interruptRequest_ );
        return connectSignalsAndRun( operationRequested.get() );
    } );
}

void LogDataWorker::checkFileChanges()
{
    ScopedLock locker( &mutex_ );
    LOG_DEBUG << "Check file changes requested";

    operationFuture_.waitForFinished();
    interruptRequest_.clear();

    operationFuture_ = QtConcurrent::run( [this, fileName = fileName_] {
        auto operationRequested = std::make_unique<CheckFileChangesOperation>(
            fileName, indexing_data_, interruptRequest_ );

        return connectSignalsAndRun( operationRequested.get() );
    } );
}

OperationResult LogDataWorker::connectSignalsAndRun( IndexOperation* operationRequested )
{
    connect( operationRequested, &IndexOperation::indexingProgressed, this,
             &LogDataWorker::indexingProgressed );

    connect( operationRequested, &IndexOperation::indexingFinished, this,
             &LogDataWorker::onIndexingFinished, Qt::QueuedConnection );

    connect( operationRequested, &IndexOperation::fileCheckFinished, this,
             &LogDataWorker::onCheckFileFinished, Qt::QueuedConnection );

    auto result = operationRequested->run();

    operationRequested->disconnect( this );

    return result;
}

void LogDataWorker::interrupt()
{
    LOG_INFO << "Load interrupt requested";
    interruptRequest_.set();
}

void LogDataWorker::onIndexingFinished( bool result )
{
    if ( result ) {
        LOG_INFO << "finished indexing in worker thread";
        emit indexingFinished( LoadingStatus::Successful );
    }
    else {
        LOG_INFO << "indexing interrupted in worker thread";
        emit indexingFinished( LoadingStatus::Interrupted );
    }
}

void LogDataWorker::onCheckFileFinished( const MonitoredFileStatus result )
{
    LOG_INFO << "checking file finished in worker thread";
    emit checkFileChangesFinished( result );
}

//
// Operations implementation
//

FastLinePositionArray IndexOperation::parseDataBlock( LineOffset::UnderlyingType block_beginning,
                                                      const QByteArray& block,
                                                      IndexingState& state ) const
{
    state.max_length = 0;
    FastLinePositionArray line_positions;

    int pos_within_block = 0;

    const auto adjustToCharWidth = [&state]( auto pos ) {
        return static_cast<int>( pos ) - state.encodingParams.getBeforeCrOffset();
    };

    const auto expandTabs = [&]( const char* start_in_line, auto line_size ) {
        auto tab_search_start = start_in_line;
        auto next_tab = reinterpret_cast<const char*>(
            std::memchr( tab_search_start, '\t', static_cast<size_t>( line_size ) ) );
        while ( next_tab != nullptr ) {
            pos_within_block = adjustToCharWidth( next_tab - block.data() );

            LOG_DEBUG << "Tab at " << pos_within_block;

            state.additional_spaces
                += TabStop
                   - ( static_cast<int>( ( block_beginning - state.pos ) + pos_within_block
                                         + state.additional_spaces )
                       % TabStop )
                   - 1;

            const auto tab_substring_size = static_cast<int>( next_tab - tab_search_start );
            line_size -= tab_substring_size;
            tab_search_start = next_tab + 1;
            if ( line_size > 0 ) {
                next_tab = reinterpret_cast<const char*>(
                    std::memchr( tab_search_start, '\t', static_cast<size_t>( line_size ) ) );
            }
            else {
                next_tab = nullptr;
            }
        }
    };

    while ( pos_within_block != -1 ) {
        pos_within_block = qMax( static_cast<int>( state.pos - block_beginning ), 0 );

        // Looking for the next \n, expanding tabs in the process

        const auto search_start = block.data() + pos_within_block;
        const auto search_line_size = block.size() - pos_within_block;

        if ( search_line_size > 0 ) {
            const auto next_line_feed = reinterpret_cast<const char*>(
                std::memchr( search_start, '\n', static_cast<size_t>( search_line_size ) ) );

            if ( next_line_feed != nullptr ) {
                expandTabs( search_start, next_line_feed - search_start );
                pos_within_block = adjustToCharWidth( next_line_feed - block.data() );
                LOG_DEBUG << "LF at " << pos_within_block;
            }
            else {
                expandTabs( search_start, search_line_size );
                pos_within_block = -1;
            }
        }
        else {
            pos_within_block = -1;
        }

        // When a end of line has been found...
        if ( pos_within_block != -1 ) {
            state.end = pos_within_block + block_beginning;
            const auto length = state.end - state.pos + state.additional_spaces;
            if ( length > state.max_length )
                state.max_length = length;

            state.pos = state.end + state.encodingParams.lineFeedWidth;
            state.additional_spaces = 0;
            line_positions.append( LineOffset( state.pos ) );
        }
    }

    return line_positions;
}

void IndexOperation::guessEncoding( const QByteArray& block,
                                    IndexingData::MutateAccessor& scopedAccessor,
                                    IndexingState& state ) const
{
    if ( !state.encodingGuess ) {
        state.encodingGuess = EncodingDetector::getInstance().detectEncoding( block );
        LOG_INFO << "Encoding guess " << state.encodingGuess->name().toStdString();
    }

    if ( !state.fileTextCodec ) {
        state.fileTextCodec = scopedAccessor.getForcedEncoding();

        if ( !state.fileTextCodec ) {
            state.fileTextCodec = scopedAccessor.getEncodingGuess();
        }

        if ( !state.fileTextCodec ) {
            state.fileTextCodec = state.encodingGuess;
        }

        state.encodingParams = EncodingParameters( state.fileTextCodec );
        LOG_INFO << "Encoding " << state.fileTextCodec->name().toStdString() << ", Char width "
                 << state.encodingParams.lineFeedWidth;
    }
}

void IndexOperation::doIndex( LineOffset initialPosition )
{
    QFile file( fileName_ );

    if ( !( file.isOpen() || file.open( QIODevice::ReadOnly ) ) ) {
        // TODO: Check that the file is seekable?
        // If the file cannot be open, we do as if it was empty
        LOG_WARNING << "Cannot open file " << fileName_.toStdString();

        IndexingData::MutateAccessor scopedAccessor{ &indexing_data_ };

        scopedAccessor.clear();
        scopedAccessor.setEncodingGuess( QTextCodec::codecForLocale() );

        emit indexingProgressed( 100 );
        return;
    }

    IndexingState state;
    state.pos = initialPosition.get();
    state.file_size = file.size();

    {
        IndexingData::ConstAccessor scopedAccessor{ &indexing_data_ };

        state.fileTextCodec = scopedAccessor.getForcedEncoding();
        if ( !state.fileTextCodec ) {
            state.fileTextCodec = scopedAccessor.getEncodingGuess();
        }

        state.encodingGuess = scopedAccessor.getEncodingGuess();
    }

    const auto& config = Configuration::get();
    const auto prefetchBufferSize = static_cast<size_t>( config.indexReadBufferSizeMb() );

    using namespace std::chrono;
    using clock = high_resolution_clock;
    milliseconds ioDuration{};

    const auto indexingStartTime = clock::now();

    tbb::flow::graph indexingGraph;
    using BlockData = std::pair<LineOffset::UnderlyingType, QByteArray>;

    QThreadPool localThreadPool;
    localThreadPool.setMaxThreadCount( 1 );

    auto blockReaderAsync = tbb::flow::async_node<tbb::flow::continue_msg, BlockData>(
        indexingGraph, tbb::flow::serial,
        [this, &localThreadPool, &file, &ioDuration]( const auto&, auto& gateway ) {
            gateway.reserve_wait();

            QtConcurrent::run(
                &localThreadPool, [this, &file, &ioDuration, gw = std::ref( gateway )] {
                    while ( !file.atEnd() ) {

                        if ( interruptRequest_ ) {
                            break;
                        }
                        QByteArray readBuffer( IndexingBlockSize, Qt::Uninitialized );

                        BlockData blockData;
                        blockData.first = file.pos();
                        clock::time_point ioT1 = clock::now();
                        const auto readBytes = file.read( readBuffer.data(), readBuffer.size() );

                        if ( readBytes < 0 ) {
                            LOG_ERROR << "Reading past the end of file";
                            break;
                        }

                        blockData.second = readBuffer.left( static_cast<int>( readBytes ) );
                        clock::time_point ioT2 = clock::now();

                        ioDuration += duration_cast<milliseconds>( ioT2 - ioT1 );

                        LOG_DEBUG << "Sending block " << blockData.first << " size "
                                  << blockData.second.size();

                        while ( !gw.get().try_put( blockData ) ) {
                            QThread::msleep( 1 );
                        }
                    }

                    auto lastBlock = std::make_pair( -1, QByteArray{} );
                    while ( !gw.get().try_put( lastBlock ) ) {
                        QThread::msleep( 1 );
                    }

                    gw.get().release_wait();
                } );
        } );

    auto blockPrefetcher = tbb::flow::limiter_node<BlockData>( indexingGraph, prefetchBufferSize );
    auto blockQueue = tbb::flow::queue_node<BlockData>( indexingGraph );

    auto blockParser = tbb::flow::function_node<BlockData, tbb::flow::continue_msg>(
        indexingGraph, 1, [this, &state]( const BlockData& blockData ) {
            const auto& block_beginning = blockData.first;
            const auto& block = blockData.second;

            LOG_DEBUG << "Indexing block " << block_beginning << " start";

            if ( block_beginning < 0 ) {
                return tbb::flow::continue_msg{};
            }

            IndexingData::MutateAccessor scopedAccessor{ &indexing_data_ };

            guessEncoding( block, scopedAccessor, state );

            if ( !block.isEmpty() ) {
                const auto line_positions = parseDataBlock( block_beginning, block, state );
                auto max_length = state.max_length;
                if ( max_length > std::numeric_limits<LineLength::UnderlyingType>::max() ) {
                    LOG_ERROR << "Too long lines " << max_length;
                    max_length = std::numeric_limits<LineLength::UnderlyingType>::max();
                }

                scopedAccessor.addAll(
                    block, LineLength( static_cast<LineLength::UnderlyingType>( max_length ) ),
                    line_positions, state.encodingGuess );

                // Update the caller for progress indication
                const auto progress = ( state.file_size > 0 )
                                          ? calculateProgress( state.pos, state.file_size )
                                          : 100;
                emit indexingProgressed( progress );
            }
            else {
                scopedAccessor.setEncodingGuess( state.encodingGuess );
            }

            LOG_DEBUG << "Indexing block " << block_beginning << " done";

            return tbb::flow::continue_msg{};
        } );

    tbb::flow::make_edge( blockReaderAsync, blockPrefetcher );
    tbb::flow::make_edge( blockPrefetcher, blockQueue );
    tbb::flow::make_edge( blockQueue, blockParser );
    tbb::flow::make_edge( blockParser, blockPrefetcher.decrement );

    file.seek( state.pos );
    blockReaderAsync.try_put( tbb::flow::continue_msg{} );
    indexingGraph.wait_for_all();
    localThreadPool.waitForDone();

    IndexingData::MutateAccessor scopedAccessor{ &indexing_data_ };

    LOG_DEBUG << "Indexed up to " << state.pos;

    // Check if there is a non LF terminated line at the end of the file
    if ( !interruptRequest_ && state.file_size > state.pos ) {
        LOG_WARNING << "Non LF terminated file, adding a fake end of line";

        FastLinePositionArray line_position;
        line_position.append( LineOffset( state.file_size + 1 ) );
        line_position.setFakeFinalLF();

        scopedAccessor.addAll( {}, 0_length, line_position, state.encodingGuess );
    }

    const auto endFilePos = file.pos();
    file.reset();
    QByteArray hashBuffer( IndexingBlockSize, Qt::Uninitialized );
    const auto headerHashSize = file.read( hashBuffer.data(), hashBuffer.size() );
    FileDigest fastHashDigest;
    fastHashDigest.addData( hashBuffer.data(), static_cast<size_t>( headerHashSize ) );

    scopedAccessor.setHeaderHash( fastHashDigest.digest(), headerHashSize );

    if ( endFilePos <= hashBuffer.size() ) {
        scopedAccessor.setTailHash( fastHashDigest.digest(), 0, headerHashSize );
    }
    else {
        const auto tailHashOffset = endFilePos - hashBuffer.size();
        file.seek( tailHashOffset );
        const auto tailHashSize = file.read( hashBuffer.data(), hashBuffer.size() );
        fastHashDigest.reset();
        fastHashDigest.addData( hashBuffer.data(), static_cast<size_t>( tailHashSize ) );
        scopedAccessor.setTailHash( fastHashDigest.digest(), tailHashOffset, tailHashSize );
    }

    const auto indexingEndTime = high_resolution_clock::now();
    const auto duration = duration_cast<milliseconds>( indexingEndTime - indexingStartTime );

    LOG_INFO << "Indexing done, took " << duration << ", io " << ioDuration;
    LOG_INFO << "Index size "
             << readableSize( static_cast<uint64_t>( scopedAccessor.allocatedSize() ) );
    LOG_INFO << "Indexed lines " << scopedAccessor.getNbLines();
    LOG_INFO << "Max line " << scopedAccessor.getMaxLength();
    LOG_INFO << "Indexing perf "
             << ( 1000.f * static_cast<float>( state.file_size )
                  / static_cast<float>( duration.count() ) )
                    / ( 1024 * 1024 )
             << " MiB/s";

    if ( interruptRequest_ ) {
        scopedAccessor.clear();
    }

    if ( !scopedAccessor.getEncodingGuess() ) {
        scopedAccessor.setEncodingGuess( QTextCodec::codecForLocale() );
    }
}

// Called in the worker thread's context
OperationResult FullIndexOperation::run()
{
    LOG_DEBUG << "FullIndexOperation::run(), file " << fileName_.toStdString();

    LOG_DEBUG << "FullIndexOperation: Starting the count...";

    emit indexingProgressed( 0 );

    {
        IndexingData::MutateAccessor scopedAccessor{ &indexing_data_ };
        scopedAccessor.clear();
        scopedAccessor.forceEncoding( forcedEncoding_ );
    }

    doIndex( 0_offset );

    LOG_DEBUG << "FullIndexOperation: ... finished counting."
                 "interrupt = "
              << static_cast<bool>( interruptRequest_ );

    const auto result = interruptRequest_ ? false : true;
    emit indexingFinished( result );
    return result;
}

OperationResult PartialIndexOperation::run()
{
    LOG_DEBUG << "PartialIndexOperation::run(), file " << fileName_.toStdString();

    const auto initial_position
        = LineOffset( IndexingData::ConstAccessor{ &indexing_data_ }.getIndexedSize() );

    LOG_DEBUG << "PartialIndexOperation: Starting the count at " << initial_position << " ...";

    emit indexingProgressed( 0 );

    doIndex( initial_position );

    LOG_DEBUG << "PartialIndexOperation: ... finished counting.";

    const auto result = interruptRequest_ ? false : true;
    emit indexingFinished( result );
    return result;
}

OperationResult CheckFileChangesOperation::run()
{
    LOG_INFO << "CheckFileChangesOperation::run(), file " << fileName_.toStdString();
    const auto result = doCheckFileChanges();
    emit fileCheckFinished( result );
    return result;
}

MonitoredFileStatus CheckFileChangesOperation::doCheckFileChanges()
{
    QFileInfo info( fileName_ );
    const auto indexedHash = IndexingData::ConstAccessor{ &indexing_data_ }.getHash();
    const auto realFileSize = info.size();

    if ( realFileSize == 0 || realFileSize < indexedHash.size ) {
        LOG_INFO << "File truncated";
        return MonitoredFileStatus::Truncated;
    }
    else {
        QFile file( fileName_ );

        QByteArray buffer{ IndexingBlockSize, Qt::Uninitialized };

        bool isFileModified = false;
        const auto& config = Configuration::get();

        if ( !file.isOpen() && !file.open( QIODevice::ReadOnly ) ) {
            LOG_INFO << "File failed to open";
            return MonitoredFileStatus::Truncated;
        }

        const auto getDigest = [&file, &buffer]( const qint64 indexedSize ) {
            FileDigest fileDigest;
            auto readSize = 0ll;
            auto totalSize = 0ll;
            do {
                const auto bytesToRead
                    = std::min( static_cast<qint64>( buffer.size() ), indexedSize - totalSize );
                readSize = file.read( buffer.data(), bytesToRead );

                if ( readSize > 0 ) {
                    fileDigest.addData( buffer.data(), static_cast<size_t>( readSize ) );
                    totalSize += readSize;
                }

            } while ( readSize > 0 && totalSize < indexedSize );

            return fileDigest.digest();
        };
        if ( config.fastModificationDetection() ) {
            const auto headerDigest = getDigest( indexedHash.headerSize );

            LOG_INFO << "indexed header xxhash " << indexedHash.headerDigest;
            LOG_INFO << "current header xxhash " << headerDigest << ", size "
                     << indexedHash.headerSize;

            isFileModified = headerDigest != indexedHash.headerDigest;

            if ( !isFileModified ) {
                file.seek( indexedHash.tailOffset );
                const auto tailDigest = getDigest( indexedHash.tailSize );

                LOG_INFO << "indexed tail xxhash " << indexedHash.tailDigest;
                LOG_INFO << "current tail xxhash " << tailDigest << ", size "
                         << indexedHash.tailSize;

                isFileModified = tailDigest != indexedHash.tailDigest;
            }
        }
        else {

            const auto realHashDigest = getDigest( indexedHash.size );

            LOG_INFO << "indexed xxhash " << indexedHash.fullDigest;
            LOG_INFO << "current xxhash " << realHashDigest;

            isFileModified = realHashDigest != indexedHash.fullDigest;
        }

        if ( isFileModified ) {
            LOG_INFO << "File changed in indexed range";
            return MonitoredFileStatus::Truncated;
        }
        else if ( realFileSize > indexedHash.size ) {
            LOG_INFO << "New data on disk";
            return MonitoredFileStatus::DataAdded;
        }
        else {
            LOG_INFO << "No change in file";
            return MonitoredFileStatus::Unchanged;
        }
    }
}
