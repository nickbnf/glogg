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

namespace
{
    struct PartialSearchResults
    {
        PartialSearchResults() : maxLength{} {}

        SearchResultArray matchingLines;
        LineLength maxLength;
    };

    PartialSearchResults filterLines(const QRegularExpression& regex,
                                     const QStringList& lines,
                                     LineNumber chunkStart,
                                     int proc, int procs)
    {
        PartialSearchResults results;
        for (int i = proc; i < lines.size(); i += procs) {
            const auto& l = lines.at(i);
            if (regex.match(l).hasMatch()) {
                results.maxLength = qMax(results.maxLength, AbstractLogData::getUntabifiedLength(l));
                results.matchingLines.emplace_back(chunkStart + LinesCount(i));
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
    matches_.insert( std::end( matches_ ),
            std::begin( matches ), std::end( matches ) );
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

    SearchResultArray::iterator i = matches_.end();
    while ( i != matches_.begin() ) {
        i--;
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
    while ( (operationRequested_ != NULL) )
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
    while ( (operationRequested_ != NULL) )
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
        while ( (operationRequested_ != NULL) )
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
        while ( !terminate_ && (operationRequested_ == NULL) )
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
            operationRequested_ = NULL;
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
    SearchResultArray currentList;

    std::vector<QRegularExpression> regexp;
    if ( config->useParallelSearch() ) {
        for ( int core = 0; core < QThread::idealThreadCount(); ++core ) {
            regexp.emplace_back( regexp_.pattern(), regexp_.patternOptions() );
            regexp.back().optimize();
        }
    }

    const auto nbLinesInChunk = LinesCount(config->searchReadBufferSizeLines());

    // Ensure no re-alloc will be done
    currentList.reserve( nbLinesInChunk.get() );

    LOG(logDEBUG) << "Searching from line " << initialLine << " to " << nbSourceLines;

    if (initialLine < startLine_) {
        initialLine = startLine_;
    }

    const auto endLine = qMin( LineNumber( nbSourceLines.get() ), endLine_ );

    for ( auto chunkStart = initialLine; chunkStart < endLine; chunkStart = chunkStart + nbLinesInChunk ) {
        if ( *interruptRequested_ )
            break;

        const int percentage = ( chunkStart - initialLine ).get() * 100 / ( endLine - initialLine ).get();
        emit searchProgressed( nbMatches, percentage );

        const auto linesInChunk = LinesCount( qMin( nbLinesInChunk.get(), ( endLine - chunkStart ).get() ) );
        const auto lines = sourceLogData_->getLines( chunkStart, linesInChunk );

        LOG(logDEBUG) << "Chunk starting at " << chunkStart <<
             ", " << lines.size() << " lines read.";

        if ( regexp.size() > 0 ) {
            std::vector<QFuture<PartialSearchResults>> partialResults;
            for ( auto i=0; i < static_cast<int>( regexp.size() ); ++i ) {
                partialResults.push_back(QtConcurrent::run( filterLines, regexp[i], lines,
                                                           chunkStart, i, static_cast<int>( regexp.size() ) ) );
            }

            for (auto& f: partialResults) {
                f.waitForFinished();
                auto result = f.result();
                currentList.insert(currentList.end(), result.matchingLines.begin(), result.matchingLines.end());
                maxLength = qMax(maxLength, f.result().maxLength);
                nbMatches += LinesCount( static_cast<LinesCount::UnderlyingType>( f.result().matchingLines.size() ) );
            }
            std::sort(currentList.begin(), currentList.end());
        }
        else {
            auto matchResults = filterLines(regexp_, lines, chunkStart, 0, 1);
            currentList = std::move( matchResults.matchingLines );
            maxLength = qMax( maxLength, matchResults.maxLength );
            nbMatches += LinesCount( static_cast<LinesCount::UnderlyingType>( currentList.size() ) );
        }

        // After each block, copy the data to shared data
        // and update the client
        searchData.addAll( maxLength, currentList, LinesCount( chunkStart.get() ) + linesInChunk );
        currentList.clear();
    }

    emit searchProgressed( nbMatches, 100 );
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
