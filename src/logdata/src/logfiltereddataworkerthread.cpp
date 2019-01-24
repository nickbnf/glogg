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
#include <QtConcurrent>

#include "log.h"

#include "logfiltereddataworkerthread.h"
#include "logdata.h"

#include "persistentinfo.h"
#include "configuration.h"

#include <moodycamel/blockingconcurrentqueue.h>

#include <chrono>

namespace
{
    struct PartialSearchResults
    {
        PartialSearchResults() = default;

        PartialSearchResults( const PartialSearchResults& ) = delete;
        PartialSearchResults( PartialSearchResults&& ) = default;

        PartialSearchResults& operator =( const PartialSearchResults& ) = delete;
        PartialSearchResults& operator =( PartialSearchResults&& ) = default;

        SearchResultArray matchingLines;
        LineLength maxLength;

        LineNumber chunkStart;
        LinesCount processedLines;
    };

    struct SearchBlockData
    {
        SearchBlockData() = default;

        SearchBlockData( const SearchBlockData& ) = delete;
        SearchBlockData( SearchBlockData&& ) = default;

        SearchBlockData& operator =( const SearchBlockData& ) = delete;
        SearchBlockData& operator =( SearchBlockData&& ) = default;

        LineNumber chunkStart;
        std::vector<QString> lines;
        PartialSearchResults results;
    };

    PartialSearchResults filterLines( const QRegularExpression& regex,
                                     const std::vector<QString>& lines,
                                     LineNumber chunkStart )
    {
        LOG( logDEBUG ) << "Filter lines at " << chunkStart;
        PartialSearchResults results;
        results.chunkStart = chunkStart;
        results.processedLines = LinesCount( lines.size() );
        for ( size_t i = 0; i < lines.size(); ++i ) {
            const auto& l = lines.at( i );
            if ( regex.match(l).hasMatch() ) {
                results.maxLength = qMax( results.maxLength, AbstractLogData::getUntabifiedLength( l ) );
                results.matchingLines.emplace_back( chunkStart + LinesCount( i ) );
            }
        }
        return results;
    }
}

void SearchData::getAll( LineLength* length, SearchResultArray* matches,
        LinesCount* lines) const
{
    QMutexLocker locker( &dataMutex_ );

    *length  = maxLength_;
    *lines   = nbLinesProcessed_;

    // This is a copy (potentially slow)
    const auto originalSize = matches->size();
    matches->insert(matches->end(),
                    matches_.begin() + originalSize,
                    matches_.end());

    //*matches = matches_;
}

void SearchData::setAll( LineLength length,
        SearchResultArray&& matches )
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_  = length;
    matches_    = matches;
}

void SearchData::addAll( LineLength length,
        const SearchResultArray& matches, LinesCount lines )
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_        = qMax( maxLength_, length );
    nbLinesProcessed_ = lines;

    // This does a copy as we want the final array to be
    // linear.
    const auto originalSize = matches_.size();
    matches_.insert( std::end( matches_ ),
            std::begin( matches ), std::end( matches ) );

    std::inplace_merge(matches_.begin(), matches_.begin() + originalSize, matches_.end());
}

LinesCount SearchData::getNbMatches() const
{
    QMutexLocker locker( &dataMutex_ );

    return LinesCount( static_cast<LinesCount::UnderlyingType> ( matches_.size() ) );
}

// This function starts searching from the end since we use it
// to remove the final match.
void SearchData::deleteMatch( LineNumber line )
{
    QMutexLocker locker( &dataMutex_ );

    auto i = matches_.end();
    while ( i != matches_.begin() ) {
        --i;
        const auto this_line = i->lineNumber();
        if ( this_line == line ) {
            matches_.erase(i);
            break;
        }
        // Exit if we have passed the line number to look for.
        if ( this_line < line )
            break;
    }
}

void SearchData::clear()
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_        = LineLength(0);
    nbLinesProcessed_ = LinesCount(0);
    matches_.clear();
}

LogFilteredDataWorkerThread::LogFilteredDataWorkerThread(
        const LogData* sourceLogData )
    : QThread(), mutex_(), operationRequestedCond_(), nothingToDoCond_(), searchData_()
{
    operationRequested_ = nullptr;
    sourceLogData_ = sourceLogData;
}

LogFilteredDataWorkerThread::~LogFilteredDataWorkerThread()
{
    {
        QMutexLocker locker( &mutex_ );
        terminate_.set();
        operationRequestedCond_.wakeAll();
    }
    wait();
}

void LogFilteredDataWorkerThread::search(const QRegularExpression& regExp,
                                         LineNumber startLine, LineNumber endLine)
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "Search requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != nullptr) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_.clear();
    operationRequested_ = new FullSearchOperation( sourceLogData_,
            regExp, startLine, endLine, &interruptRequested_ );
    operationRequestedCond_.wakeAll();
}

void LogFilteredDataWorkerThread::updateSearch(const QRegularExpression &regExp,
                                               LineNumber startLine, LineNumber endLine, LineNumber position )
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "Search requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != nullptr) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_.clear();
    operationRequested_ = new UpdateSearchOperation( sourceLogData_,
            regExp, startLine, endLine,  &interruptRequested_, position );
    operationRequestedCond_.wakeAll();
}

void LogFilteredDataWorkerThread::interrupt()
{
    LOG(logDEBUG) << "Search interruption requested";

    interruptRequested_.set();

    // We wait for the interruption to be done
    {
        QMutexLocker locker( &mutex_ );
        while ( (operationRequested_ != nullptr) )
            nothingToDoCond_.wait( &mutex_ );
    }
}

// This will do an atomic copy of the object
void LogFilteredDataWorkerThread::getSearchResult(
        LineLength* maxLength, SearchResultArray* searchMatches, LinesCount* nbLinesProcessed )
{
    searchData_.getAll( maxLength, searchMatches, nbLinesProcessed );
}

// This is the thread's main loop
void LogFilteredDataWorkerThread::run()
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
            connect( operationRequested_, &SearchOperation::searchProgressed,
                    this, &LogFilteredDataWorkerThread::searchProgressed );

            // Run the search operation
            operationRequested_->start( searchData_ );

            LOG(logDEBUG) << "... finished copy in workerThread.";

            emit searchFinished();
            delete operationRequested_;
            operationRequested_ = nullptr;
            nothingToDoCond_.wakeAll();
        }
    }
}

//
// Operations implementation
//

SearchOperation::SearchOperation( const LogData* sourceLogData,
        const QRegularExpression& regExp,
        LineNumber startLine, LineNumber endLine,
        AtomicFlag* interruptRequest )
    : regexp_( regExp ), sourceLogData_( sourceLogData ), startLine_(startLine), endLine_(endLine)

{
    interruptRequested_ = interruptRequest;
}

void SearchOperation::doSearch( SearchData& searchData, LineNumber initialLine )
{
    static std::shared_ptr<Configuration> config =
        Persistent<Configuration>( "settings" );

    const auto nbSourceLines = sourceLogData_->getNbLine();
    LineLength maxLength = 0_length;
    LinesCount nbMatches = searchData.getNbMatches();

    const auto nbLinesInChunk = LinesCount(config->searchReadBufferSizeLines());

    LOG( logDEBUG ) << "Searching from line " << initialLine << " to " << nbSourceLines;

    if (initialLine < startLine_) {
        initialLine = startLine_;
    }

    const auto endLine = qMin( LineNumber( nbSourceLines.get() ), endLine_ );

    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    QSemaphore searchCompleted;
    QSemaphore blocksDone;

    using SearchBlockQueue = moodycamel::BlockingConcurrentQueue<SearchBlockData>;
    using ProcessMatchQueue = moodycamel::BlockingConcurrentQueue<PartialSearchResults>;

    SearchBlockQueue searchBlockQueue;
    ProcessMatchQueue processMatchQueue;

    std::vector<QFuture<void>> matchers;

    const auto matchingThreadsCount = config->useParallelSearch() ? qMax( 1, QThread::idealThreadCount() - 2 ) : 1;

    LOG(logINFO) << "Using " << matchingThreadsCount << " matching threads";

    auto localThreadPool = std::make_unique<QThreadPool>();
    localThreadPool->setMaxThreadCount( matchingThreadsCount + 2 );

    const auto makeMatcher = [ this, &searchBlockQueue, &processMatchQueue, pool = localThreadPool.get() ] ( size_t index )
    {
        // copy and optimize regex for each thread
        auto regexp = QRegularExpression{ regexp_.pattern(), regexp_.patternOptions() };
        regexp.optimize();
        return QtConcurrent::run( pool, [ regexp, index, &searchBlockQueue, &processMatchQueue, this ]()
        {
            for(;;) {
                SearchBlockData blockData;
                searchBlockQueue.wait_dequeue( blockData );

                LOG( logDEBUG ) << "Searcher " << index << " " << blockData.chunkStart;

                const auto lastBlock = blockData.lines.empty();

                if ( !lastBlock ) {
                    blockData.results = filterLines( regexp, blockData.lines, blockData.chunkStart );
                }

                LOG( logDEBUG ) << "Searcher " << index << " sending matches " << blockData.results.matchingLines.size();

                processMatchQueue.enqueue( std::move( blockData.results ) );

                if ( lastBlock ) {
                    LOG( logDEBUG ) << "Searcher " << index << " last block";
                    return;
                }
            }
        });
    };

    for ( int i = 0; i < matchingThreadsCount; ++i ) {
        matchers.emplace_back( makeMatcher(i) );
    }

    auto processMatches = QtConcurrent::run( localThreadPool.get(), [&]()
    {
        size_t matchersDone = 0;
        for(;;) {
            PartialSearchResults matchResults;
            processMatchQueue.wait_dequeue( matchResults );
        
            LOG( logDEBUG ) << "Combining match results from " << matchResults.chunkStart;

            if ( matchResults.processedLines.get() ) {

                maxLength = qMax( maxLength, matchResults.maxLength );
                nbMatches += LinesCount( static_cast<LinesCount::UnderlyingType>( matchResults.matchingLines.size() ) );

                const auto processedLines = LinesCount{matchResults.chunkStart.get() + matchResults.processedLines.get()};

                // After each block, copy the data to shared data
                // and update the client
                searchData.addAll( maxLength, matchResults.matchingLines, processedLines );

                LOG( logDEBUG ) << "done Searching chunk starting at " << matchResults.chunkStart <<
                    ", " << matchResults.processedLines << " lines read.";

                blocksDone.release( matchResults.processedLines.get() );
            }
            else {
                matchersDone++;
            }
            
            if ( matchersDone == matchers.size() ) {
                searchCompleted.release();
                return;
            }
        }
    });

    blocksDone.release( nbLinesInChunk.get() * ( matchers.size() + 1 ) );

    int reportedPercentage = 0;
    auto reportedMatches = nbMatches;

    for ( auto chunkStart = initialLine; chunkStart < endLine; chunkStart = chunkStart + nbLinesInChunk ) {
        if ( *interruptRequested_ )
            break;

        const int percentage = ( chunkStart - initialLine ).get() * 100 / ( endLine - initialLine ).get();

        if ( percentage != reportedPercentage || nbMatches != reportedMatches ) {

            emit searchProgressed( nbMatches, percentage, initialLine );

            reportedPercentage = percentage;
            reportedMatches = nbMatches;
        }

        LOG( logDEBUG ) << "Reading chunk starting at " << chunkStart;

        const auto linesInChunk = LinesCount( qMin( nbLinesInChunk.get(), ( endLine - chunkStart ).get() ) );
        auto lines = sourceLogData_->getLines( chunkStart, linesInChunk );

        LOG( logDEBUG ) << "Sending chunk starting at " << chunkStart <<
             ", " << lines.size() << " lines read.";

        blocksDone.acquire( lines.size() );

        searchBlockQueue.enqueue( { chunkStart, std::move(lines), PartialSearchResults{} } );

        LOG( logDEBUG ) << "Sent chunk starting at " << chunkStart <<
             ", " << lines.size() << " lines read.";
    }

    for ( size_t i = 0; i < matchers.size(); ++ i ) {
        searchBlockQueue.enqueue( { endLine, std::vector<QString>{}, PartialSearchResults{} } );
    }
    
    searchCompleted.acquire();

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>( t2 - t1 ).count();

    LOG( logINFO ) << "Searching done, took " << duration / 1000.f << " ms";
    LOG( logINFO ) << "Searching perf "
                   << static_cast<uint32_t>(std::floor(1000 * 1000.f * (endLine - initialLine).get() / duration)) << " lines/s";

    emit searchProgressed( nbMatches, 100, initialLine );
}

// Called in the worker thread's context
void FullSearchOperation::start( SearchData& searchData )
{
    // Clear the shared data
    searchData.clear();

    doSearch( searchData, 0_lnum );
}

// Called in the worker thread's context
void UpdateSearchOperation::start( SearchData& searchData )
{
    auto initial_line = initialPosition_;

    if ( initial_line.get() >= 1 ) {
        // We need to re-search the last line because it might have
        // been updated (if it was not LF-terminated)
        --initial_line;
        // In case the last line matched, we don't want it to match twice.
        searchData.deleteMatch( initial_line );
    }

    doSearch( searchData, initial_line );
}
