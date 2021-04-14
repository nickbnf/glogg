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

#include <QtConcurrent>

#include "log.h"

#include "logdata.h"
#include "logfiltereddataworker.h"

#include "configuration.h"

#include <immer/box.hpp>
#include <tbb/flow_graph.h>

#include <chrono>
#include <cmath>
#include <utility>

namespace {
struct PartialSearchResults {
    PartialSearchResults() = default;

    PartialSearchResults( const PartialSearchResults& ) = delete;
    PartialSearchResults( PartialSearchResults&& ) = default;
    PartialSearchResults& operator=( const PartialSearchResults& ) = delete;
    PartialSearchResults& operator=( PartialSearchResults&& ) = default;

    SearchResultArray matchingLines;
    LineLength maxLength;

    LineNumber chunkStart;
    LinesCount processedLines;
};

struct SearchBlockData {
    SearchBlockData() = default;
    SearchBlockData( LineNumber start, std::vector<QString> blockLines )
        : chunkStart( start )
        , lines( std::move( blockLines ) )
    {
    }

    SearchBlockData( const SearchBlockData& ) = delete;
    SearchBlockData( SearchBlockData&& ) = default;

    SearchBlockData& operator=( const SearchBlockData& ) = delete;
    SearchBlockData& operator=( SearchBlockData&& ) = default;

    LineNumber chunkStart;
    std::vector<QString> lines;
};

PartialSearchResults filterLines( const QRegularExpression& regex,
                                  const std::vector<QString>& lines, LineNumber chunkStart )
{
    LOG_DEBUG << "Filter lines at " << chunkStart;
    PartialSearchResults results;
    results.chunkStart = chunkStart;
    results.processedLines = LinesCount( static_cast<LinesCount::UnderlyingType>( lines.size() ) );
    for ( size_t i = 0; i < lines.size(); ++i ) {
        const auto& l = lines.at( i );
        if ( regex.match( l ).hasMatch() ) {
            results.maxLength = qMax( results.maxLength, getUntabifiedLength( l ) );
            results.matchingLines
                = std::move( results.matchingLines )
                      .push_back( chunkStart
                                  + LinesCount( static_cast<LinesCount::UnderlyingType>( i ) ) );
        }
    }
    return results;
}
} // namespace

SearchResults SearchData::takeCurrentResults() const
{
    ScopedLock locker( &dataMutex_ );

    auto results
        = SearchResults{ matches_, std::move( newMatches_ ), maxLength_, nbLinesProcessed_ };
    newMatches_ = {};
    return results;
}

void SearchData::setAll( LineLength length, SearchResultArray&& matches )
{
    ScopedLock locker( &dataMutex_ );

    maxLength_ = length;
    matches_ = matches;
    newMatches_ = matches;
}

void SearchData::addAll( LineLength length, const SearchResultArray& matches, LinesCount lines )
{
    using std::begin;
    using std::end;

    ScopedLock locker( &dataMutex_ );

    maxLength_ = qMax( maxLength_, length );
    nbLinesProcessed_ = qMax( nbLinesProcessed_, lines );

    // This does a copy as we want the final array to be
    // linear.
    if ( !matches.empty() ) {
        lastMatchedLineNumber_ = std::max( lastMatchedLineNumber_, matches.back().lineNumber() );

        const auto insertNewMatches = [ &matches ]( SearchResultArray& oldMatches ) {
            const auto insertIt
                = std::lower_bound( begin( oldMatches ), end( oldMatches ), matches.front() );

            const auto areMatchesValid
                = insertIt == end( oldMatches ) || !( *insertIt < matches.back() );

            if ( !areMatchesValid ) {
                LOG_ERROR << "Invalid matches";
                LOG_ERROR << "insertIt: " << insertIt->lineNumber();
                LOG_ERROR << "old matches: " << oldMatches.front().lineNumber() << "-"
                          << oldMatches.back().lineNumber();
                LOG_ERROR << "new matches: " << matches.front().lineNumber() << "-"
                          << matches.back().lineNumber();
            }

            assert( areMatchesValid );

            auto insertPos = static_cast<SearchResultArray::size_type>(
                distance( begin( oldMatches ), insertIt ) );

            oldMatches = std::move( oldMatches ).insert( insertPos, matches );
        };

        insertNewMatches( matches_ );
        insertNewMatches( newMatches_ );
    }
}

LinesCount SearchData::getNbMatches() const
{
    ScopedLock locker( &dataMutex_ );

    return LinesCount( static_cast<LinesCount::UnderlyingType>( matches_.size() ) );
}

LineNumber SearchData::getLastMatchedLineNumber() const
{
    ScopedLock locker( &dataMutex_ );
    return lastMatchedLineNumber_;
}

LineNumber SearchData::getLastProcessedLine() const
{
    ScopedLock locker( &dataMutex_ );
    return LineNumber{ nbLinesProcessed_.get() };
}

void SearchData::deleteMatch( LineNumber line )
{
    ScopedLock locker( &dataMutex_ );

    auto i = matches_.size();
    while ( i != 0 ) {
        --i;
        const auto this_line = matches_.at( i ).lineNumber();
        if ( this_line == line ) {
            matches_ = std::move( matches_ ).erase( i );
            break;
        }
        // Exit if we have passed the line number to look for.
        if ( this_line < line )
            break;
    }
}

void SearchData::clear()
{
    ScopedLock locker( &dataMutex_ );

    maxLength_ = LineLength( 0 );
    nbLinesProcessed_ = LinesCount( 0 );
    lastMatchedLineNumber_ = LineNumber( 0 );
    matches_ = {};
    newMatches_ = {};
}

LogFilteredDataWorker::LogFilteredDataWorker( const LogData& sourceLogData )
    : sourceLogData_( sourceLogData )
    , mutex_()
    , searchData_()
{
}

LogFilteredDataWorker::~LogFilteredDataWorker()
{
    interruptRequested_.set();
    ScopedLock locker( &mutex_ );
    operationFuture_.waitForFinished();
}

void LogFilteredDataWorker::connectSignalsAndRun( SearchOperation* operationRequested )
{
    connect( operationRequested, &SearchOperation::searchProgressed, this,
             &LogFilteredDataWorker::searchProgressed );
    connect( operationRequested, &SearchOperation::searchFinished, this,
             &LogFilteredDataWorker::searchFinished, Qt::QueuedConnection );

    operationRequested->run( searchData_ );
    operationRequested->disconnect( this );
}

void LogFilteredDataWorker::search( const QRegularExpression& regExp, LineNumber startLine,
                                    LineNumber endLine )
{
    ScopedLock locker( &mutex_ ); // to protect operationRequested_

    LOG_INFO << "Search requested";

    operationFuture_.waitForFinished();
    interruptRequested_.clear();

    operationFuture_ = QtConcurrent::run( [ this, regExp, startLine, endLine ] {
        auto operationRequested = std::make_unique<FullSearchOperation>(
            sourceLogData_, interruptRequested_, regExp, startLine, endLine );
        connectSignalsAndRun( operationRequested.get() );
    } );
}

void LogFilteredDataWorker::updateSearch( const QRegularExpression& regExp, LineNumber startLine,
                                          LineNumber endLine, LineNumber position )
{
    ScopedLock locker( &mutex_ ); // to protect operationRequested_

    LOG_INFO << "Search update requested from " << position.get();

    operationFuture_.waitForFinished();
    interruptRequested_.clear();

    operationFuture_ = QtConcurrent::run( [ this, regExp, startLine, endLine, position ] {
        auto operationRequested = std::make_unique<UpdateSearchOperation>(
            sourceLogData_, interruptRequested_, regExp, startLine, endLine, position );
        connectSignalsAndRun( operationRequested.get() );
    } );
}

void LogFilteredDataWorker::interrupt()
{
    LOG_INFO << "Search interruption requested";
    interruptRequested_.set();
}

// This will do an atomic copy of the object
SearchResults LogFilteredDataWorker::getSearchResults() const
{
    return searchData_.takeCurrentResults();
}

//
// Operations implementation
//

SearchOperation::SearchOperation( const LogData& sourceLogData, AtomicFlag& interruptRequested,
                                  const QRegularExpression& regExp, LineNumber startLine,
                                  LineNumber endLine )

    : interruptRequested_( interruptRequested )
    , regexp_( regExp )
    , sourceLogData_( sourceLogData )
    , startLine_( startLine )
    , endLine_( endLine )

{
}

void SearchOperation::doSearch( SearchData& searchData, LineNumber initialLine )
{
    const auto nbSourceLines = sourceLogData_.getNbLine();

    LOG_INFO << "Searching from line " << initialLine << " to " << nbSourceLines;

    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    const auto& config = Configuration::get();
    const auto matchingThreadsCount = static_cast<uint32_t>( [ &config ]() {
        if ( !config.useParallelSearch() ) {
            return 1;
        }

        return qMax( 1, config.searchThreadPoolSize() == 0
                            ? QThread::idealThreadCount() - 1
                            : static_cast<int>( config.searchThreadPoolSize() ) );
    }() );

    LOG_INFO << "Using " << matchingThreadsCount << " matching threads";

    tbb::flow::graph searchGraph;

    if ( initialLine < startLine_ ) {
        initialLine = startLine_;
    }

    const auto endLine = qMin( LineNumber( nbSourceLines.get() ), endLine_ );
    const auto nbLinesInChunk = LinesCount(
        static_cast<LinesCount::UnderlyingType>( config.searchReadBufferSizeLines() ) );

    std::chrono::microseconds fileReadingDuration{ 0 };

    using BlockDataType = immer::box<SearchBlockData>;
    using PartialResultType = immer::box<PartialSearchResults>;

    auto chunkStart = initialLine;
    auto linesSource = tbb::flow::input_node<BlockDataType>(
        searchGraph,
        [ this, endLine, nbLinesInChunk, &chunkStart,
          &fileReadingDuration ]( tbb::flow_control& fc ) -> BlockDataType {
            if ( interruptRequested_ ) {
                LOG_INFO << "Block reader interrupted";
                fc.stop();
                return {};
            }

            if ( chunkStart >= endLine ) {
                fc.stop();
                return {};
            }

            const auto lineSourceStartTime = high_resolution_clock::now();

            LOG_DEBUG << "Reading chunk starting at " << chunkStart;

            const auto linesInChunk
                = LinesCount( qMin( nbLinesInChunk.get(), ( endLine - chunkStart ).get() ) );
            auto lines = sourceLogData_.getLines( chunkStart, linesInChunk );

            LOG_DEBUG << "Sending chunk starting at " << chunkStart << ", " << lines.size()
                      << " lines read.";

            auto blockData = BlockDataType{ chunkStart, std::move( lines ) };

            const auto lineSourceEndTime = high_resolution_clock::now();
            const auto chunkReadTime
                = duration_cast<microseconds>( lineSourceEndTime - lineSourceStartTime );

            LOG_DEBUG << "Sent chunk starting at " << chunkStart << ", " << blockData->lines.size()
                      << " lines read in " << static_cast<float>( chunkReadTime.count() ) / 1000.f
                      << " ms";

            chunkStart = chunkStart + nbLinesInChunk;

            fileReadingDuration += chunkReadTime;

            return blockData;
        } );

    auto blockPrefetcher
        = tbb::flow::limiter_node<BlockDataType>( searchGraph, matchingThreadsCount * 3 );

    auto lineBlocksQueue = tbb::flow::buffer_node<BlockDataType>( searchGraph );

    using RegexMatcher
        = tbb::flow::function_node<BlockDataType, PartialResultType, tbb::flow::rejecting>;

    std::vector<std::tuple<QRegularExpression, microseconds, RegexMatcher>> regexMatchers;
    for ( auto index = 0u; index < matchingThreadsCount; ++index ) {
        regexMatchers.emplace_back(
            QRegularExpression{ regexp_.pattern(), regexp_.patternOptions() }, microseconds{ 0 },
            RegexMatcher(
                searchGraph, 1, [ &regexMatchers, index ]( const BlockDataType& blockData ) {
                    QRegularExpression& regex = std::get<0>( regexMatchers.at( index ) );
                    const auto matchStartTime = high_resolution_clock::now();

                    auto results = PartialResultType(
                        filterLines( regex, blockData->lines, blockData->chunkStart ) );

                    const auto matchEndTime = high_resolution_clock::now();

                    microseconds& matchDuration = std::get<1>( regexMatchers.at( index ) );
                    matchDuration += duration_cast<microseconds>( matchEndTime - matchStartTime );
                    LOG_DEBUG << "Searcher " << index << " block " << blockData->chunkStart
                              << " sending matches " << results->matchingLines.size();
                    return results;
                } ) );
        std::get<0>( regexMatchers.back() ).optimize();
    }

    auto resultsQueue = tbb::flow::buffer_node<PartialResultType>( searchGraph );

    const auto totalLines = endLine - initialLine;
    LinesCount totalProcessedLines = 0_lcount;
    LineLength maxLength = 0_length;
    LinesCount nbMatches = searchData.getNbMatches();
    auto reportedMatches = nbMatches;
    int reportedPercentage = 0;

    std::chrono::microseconds matchCombiningDuration{ 0 };

    auto matchProcessor = tbb::flow::function_node<PartialResultType, tbb::flow::continue_msg,
                                                   tbb::flow::rejecting>(
        searchGraph, 1, [ & ]( const PartialResultType& matchResults ) {
            if ( interruptRequested_ ) {
                LOG_INFO << "Match processor interrupted";
                return tbb::flow::continue_msg{};
            }

            const auto matchProcessorStartTime = high_resolution_clock::now();

            if ( matchResults->processedLines.get() ) {

                maxLength = qMax( maxLength, matchResults->maxLength );
                nbMatches += LinesCount(
                    static_cast<LinesCount::UnderlyingType>( matchResults->matchingLines.size() ) );

                const auto processedLines = LinesCount{ matchResults->chunkStart.get()
                                                        + matchResults->processedLines.get() };

                totalProcessedLines += matchResults->processedLines;

                // After each block, copy the data to shared data
                // and update the client
                searchData.addAll( maxLength, matchResults->matchingLines, processedLines );

                LOG_DEBUG << "done Searching chunk starting at " << matchResults->chunkStart << ", "
                          << matchResults->processedLines << " lines read.";
            }

            const int percentage = calculateProgress( totalProcessedLines.get(), totalLines.get() );

            if ( percentage > reportedPercentage || nbMatches > reportedMatches ) {

                emit searchProgressed( nbMatches, std::min( 99, percentage ), initialLine );

                reportedPercentage = percentage;
                reportedMatches = nbMatches;
            }

            const auto matchProcessorEndTime = high_resolution_clock::now();
            matchCombiningDuration
                += duration_cast<microseconds>( matchProcessorEndTime - matchProcessorStartTime );

            return tbb::flow::continue_msg{};
        } );

    tbb::flow::make_edge( linesSource, blockPrefetcher );
    tbb::flow::make_edge( blockPrefetcher, lineBlocksQueue );

    for ( auto& regexMatcher : regexMatchers ) {
        tbb::flow::make_edge( lineBlocksQueue, std::get<2>( regexMatcher ) );
        tbb::flow::make_edge( std::get<2>( regexMatcher ), resultsQueue );
    }

    tbb::flow::make_edge( resultsQueue, matchProcessor );
    tbb::flow::make_edge( matchProcessor, blockPrefetcher.decrementer() );

    linesSource.activate();
    searchGraph.wait_for_all();

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    const auto duration = static_cast<float>( duration_cast<microseconds>( t2 - t1 ).count() );

    LOG_INFO << "Searching done, overall duration " << duration / 1000.f << " ms";
    LOG_INFO << "Line reading took " << static_cast<float>( fileReadingDuration.count() ) / 1000.f
             << " ms";
    LOG_INFO << "Results combining took "
             << static_cast<float>( matchCombiningDuration.count() ) / 1000.f << " ms";

    for ( const auto& regexMatcher : regexMatchers ) {
        LOG_INFO << "Matching took "
                 << static_cast<float>( std::get<1>( regexMatcher ).count() ) / 1000.f << " ms";
    }

    LOG_INFO << "Searching perf "
             << static_cast<uint32_t>( std::floor(
                    1000 * 1000.f * static_cast<float>( ( endLine - initialLine ).get() )
                    / duration ) )
             << " lines/s";

    emit searchProgressed( nbMatches, 100, initialLine );
    emit searchFinished();
}

// Called in the worker thread's context
void FullSearchOperation::run( SearchData& searchData )
{
    // Clear the shared data
    searchData.clear();

    doSearch( searchData, 0_lnum );
}

// Called in the worker thread's context
void UpdateSearchOperation::run( SearchData& searchData )
{
    auto initial_line = qMax( searchData.getLastProcessedLine(), initialPosition_ );

    if ( initial_line.get() >= 1 ) {
        // We need to re-search the last line because it might have
        // been updated (if it was not LF-terminated)
        --initial_line;
        // In case the last line matched, we don't want it to match twice.
        searchData.deleteMatch( initial_line );
    }

    doSearch( searchData, initial_line );
}
