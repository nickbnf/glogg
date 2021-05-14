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

#include <chrono>
#include <cmath>
#include <utility>

#include <QtConcurrent>

#include <tbb/flow_graph.h>

#include "configuration.h"
#include "log.h"
#include "overload_visitor.h"
#include "progress.h"

#include "hsregularexpression.h"
#include "logdata.h"

#include "logfiltereddataworker.h"

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
    SearchBlockData( LineNumber start, LogData::RawLines blockLines )
        : chunkStart( start )
        , lines( std::move( blockLines ) )
    {
    }

    SearchBlockData( const SearchBlockData& ) = delete;
    SearchBlockData( SearchBlockData&& ) = default;

    SearchBlockData& operator=( const SearchBlockData& ) = delete;
    SearchBlockData& operator=( SearchBlockData&& ) = default;

    LineNumber chunkStart;
    LogData::RawLines lines;
};

PartialSearchResults filterLines( const MatcherVariant& matcher, const std::vector<QString>& lines,
                                  LineNumber chunkStart )
{
    LOG_DEBUG << "Filter lines at " << chunkStart;
    PartialSearchResults results;
    results.chunkStart = chunkStart;
    results.processedLines = LinesCount( static_cast<LinesCount::UnderlyingType>( lines.size() ) );
    for ( auto offset = 0_lcount; offset < results.processedLines; ++offset ) {
        const auto& line = lines[ offset.get() ];

        const auto hasMatch
            = absl::visit( [ &line ]( const auto& m ) { return m.hasMatch( line ); }, matcher );

        if ( hasMatch ) {
            results.maxLength = qMax( results.maxLength, getUntabifiedLength( line ) );
            const auto lineNumber = chunkStart + offset;
            results.matchingLines.add( lineNumber.get() );
        }
    }
    return results;
}

} // namespace

SearchResults SearchData::takeCurrentResults() const
{
    ScopedLock locker( dataMutex_ );
    return SearchResults{ std::exchange( newMatches_, {} ), maxLength_, nbLinesProcessed_ };
}

void SearchData::addAll( LineLength length, const SearchResultArray& matches, LinesCount lines )
{
    ScopedLock locker( dataMutex_ );

    maxLength_ = qMax( maxLength_, length );
    nbLinesProcessed_ = qMax( nbLinesProcessed_, lines );
    nbMatches_ += LinesCount( static_cast<LinesCount::UnderlyingType>( matches.cardinality() ) );

    newMatches_ |= matches;
}

LinesCount SearchData::getNbMatches() const
{
    ScopedLock locker( dataMutex_ );

    return nbMatches_;
}

LineNumber SearchData::getLastProcessedLine() const
{
    ScopedLock locker( dataMutex_ );
    return LineNumber{ nbLinesProcessed_.get() };
}

void SearchData::deleteMatch( LineNumber line )
{
    ScopedLock locker( dataMutex_ );

    matches_.remove( line.get() );
}

void SearchData::clear()
{
    ScopedLock locker( dataMutex_ );

    maxLength_ = LineLength( 0 );
    nbLinesProcessed_ = LinesCount( 0 );
    nbMatches_ = LinesCount( 0 );
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
    ScopedLock locker( mutex_ );
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

void LogFilteredDataWorker::search( const RegularExpressionPattern& regExp, LineNumber startLine,
                                    LineNumber endLine )
{
    ScopedLock locker( mutex_ ); // to protect operationRequested_

    LOG_INFO << "Search requested";

    operationFuture_.waitForFinished();
    interruptRequested_.clear();

    operationFuture_ = QtConcurrent::run( [ this, regExp, startLine, endLine ] {
        auto operationRequested = std::make_unique<FullSearchOperation>(
            sourceLogData_, interruptRequested_, regExp, startLine, endLine );
        connectSignalsAndRun( operationRequested.get() );
    } );
}

void LogFilteredDataWorker::updateSearch( const RegularExpressionPattern& regExp,
                                          LineNumber startLine, LineNumber endLine,
                                          LineNumber position )
{
    ScopedLock locker( mutex_ ); // to protect operationRequested_

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
                                  const RegularExpressionPattern& regExp, LineNumber startLine,
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
                            ? QThread::idealThreadCount()
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

    using BlockDataType = std::shared_ptr<SearchBlockData>;
    using PartialResultType = std::shared_ptr<PartialSearchResults>;

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
            auto lines = sourceLogData_.getLinesRaw( chunkStart, linesInChunk );

            /*LOG_DEBUG << "Sending chunk starting at " << chunkStart << ", " << lines.second.size()
                      << " lines read.";*/

            auto blockData = std::make_shared<SearchBlockData>( chunkStart, std::move( lines ) );

            const auto lineSourceEndTime = high_resolution_clock::now();
            const auto chunkReadTime
                = duration_cast<microseconds>( lineSourceEndTime - lineSourceStartTime );

            /*LOG_DEBUG << "Sent chunk starting at " << chunkStart << ", " <<
               blockData->lines.second.size()
                      << " lines read in " << static_cast<float>( chunkReadTime.count() ) / 1000.f
                      << " ms";*/

            chunkStart = chunkStart + nbLinesInChunk;

            fileReadingDuration += chunkReadTime;

            return blockData;
        } );

    auto blockPrefetcher
        = tbb::flow::limiter_node<BlockDataType>( searchGraph, matchingThreadsCount * 3 );

    auto lineBlocksQueue = tbb::flow::buffer_node<BlockDataType>( searchGraph );

    using RegexMatcherNode
        = tbb::flow::function_node<BlockDataType, PartialResultType, tbb::flow::rejecting>;
    using MatcherContext = std::tuple<MatcherVariant, microseconds, RegexMatcherNode>;

    std::vector<MatcherContext> regexMatchers;
    HsRegularExpression hsRegularExpression{ regexp_ };
    const auto useHyperscanEngine = config.regexpEngine() == RegexpEngine::Hyperscan;
    for ( auto index = 0u; index < matchingThreadsCount; ++index ) {
        regexMatchers.emplace_back(
            useHyperscanEngine ? hsRegularExpression.createMatcher()
                               : MatcherVariant{ DefaultRegularExpressionMatcher( regexp_ ) },
            microseconds{ 0 },
            RegexMatcherNode(
                searchGraph, 1, [ this, &regexMatchers, index ]( const BlockDataType& blockData ) {
                    const auto& matcher = std::get<0>( regexMatchers.at( index ) );
                    const auto matchStartTime = high_resolution_clock::now();

                    auto results = std::make_shared<PartialSearchResults>(
                        filterLines( matcher, sourceLogData_.decodeLines( blockData->lines ),
                                     blockData->chunkStart ) );

                    const auto matchEndTime = high_resolution_clock::now();

                    microseconds& matchDuration = std::get<1>( regexMatchers.at( index ) );
                    matchDuration += duration_cast<microseconds>( matchEndTime - matchStartTime );
                    LOG_DEBUG << "Searcher " << index << " block " << blockData->chunkStart
                              << " sending matches " << results->matchingLines.cardinality();
                    return results;
                } ) );
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
                nbMatches += LinesCount( static_cast<LinesCount::UnderlyingType>(
                    matchResults->matchingLines.cardinality() ) );

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
    const auto duration
        = static_cast<float>( duration_cast<microseconds>( t2 - t1 ).count() ) / 1000.f;

    LOG_INFO << "Searching done, overall duration " << duration << " ms";
    LOG_INFO << "Line reading took " << static_cast<float>( fileReadingDuration.count() ) / 1000.f
             << " ms";
    LOG_INFO << "Results combining took "
             << static_cast<float>( matchCombiningDuration.count() ) / 1000.f << " ms";

    for ( const auto& regexMatcher : regexMatchers ) {
        LOG_INFO << "Matching took "
                 << static_cast<float>( std::get<1>( regexMatcher ).count() ) / 1000.f << " ms";
    }

    const auto totalFileSize = sourceLogData_.getFileSize();

    LOG_INFO << "Searching perf "
             << static_cast<uint32_t>( std::floor(
                    1000.f * static_cast<float>( ( endLine - initialLine ).get() ) / duration ) )
             << " lines/s";
    LOG_INFO << "Searching io perf "
             << ( 1000.f * static_cast<float>( totalFileSize ) / static_cast<float>( duration ) )
                    / ( 1024 * 1024 )
             << " MiB/s";

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
