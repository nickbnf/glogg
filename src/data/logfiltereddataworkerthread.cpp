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

#include "logfiltereddataworkerthread.h"
#include "logdata.h"

// Number of lines in each chunk to read
const int SearchOperation::nbLinesInChunk = 5000;

void SearchData::getAll( int* length, SearchResultArray* matches,
        qint64* lines) const
{
    QMutexLocker locker( &dataMutex_ );

    *length  = maxLength_;
    *lines   = nbLinesProcessed_;

    // This is a copy (potentially slow)
    *matches = matches_;
}

void SearchData::setAll( int length,
        SearchResultArray&& matches )
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_  = length;
    matches_    = matches;
}

void SearchData::addAll( int length,
        const SearchResultArray& matches, LineNumber lines )
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_        = qMax( maxLength_, length );
    nbLinesProcessed_ = lines;

    // This does a copy as we want the final array to be
    // linear.
    matches_.insert( std::end( matches_ ),
            std::begin( matches ), std::end( matches ) );
}

LineNumber SearchData::getNbMatches() const
{
    QMutexLocker locker( &dataMutex_ );

    return matches_.size();
}

// This function starts searching from the end since we use it
// to remove the final match.
void SearchData::deleteMatch( LineNumber line )
{
    QMutexLocker locker( &dataMutex_ );

    SearchResultArray::iterator i = matches_.end();
    while ( i != matches_.begin() ) {
        i--;
        const LineNumber this_line = i->lineNumber();
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

    maxLength_        = 0;
    nbLinesProcessed_ = 0;
    matches_.clear();
}

LogFilteredDataWorkerThread::LogFilteredDataWorkerThread(
        const LogData* sourceLogData )
    : QThread(), mutex_(), operationRequestedCond_(), nothingToDoCond_(), searchData_()
{
    terminate_          = false;
    interruptRequested_ = false;
    operationRequested_ = NULL;

    sourceLogData_ = sourceLogData;
}

LogFilteredDataWorkerThread::~LogFilteredDataWorkerThread()
{
    {
        QMutexLocker locker( &mutex_ );
        terminate_ = true;
        operationRequestedCond_.wakeAll();
    }
    wait();
}

void LogFilteredDataWorkerThread::search( const QRegularExpression& regExp )
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "Search requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != NULL) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_ = false;
    operationRequested_ = new FullSearchOperation( sourceLogData_,
            regExp, &interruptRequested_ );
    operationRequestedCond_.wakeAll();
}

void LogFilteredDataWorkerThread::updateSearch(const QRegularExpression &regExp, qint64 position )
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "Search requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != NULL) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_ = false;
    operationRequested_ = new UpdateSearchOperation( sourceLogData_,
            regExp, &interruptRequested_, position );
    operationRequestedCond_.wakeAll();
}

void LogFilteredDataWorkerThread::interrupt()
{
    LOG(logDEBUG) << "Search interruption requested";

    // No mutex here, setting a bool is probably atomic!
    interruptRequested_ = true;

    // We wait for the interruption to be done
    {
        QMutexLocker locker( &mutex_ );
        while ( (operationRequested_ != NULL) )
            nothingToDoCond_.wait( &mutex_ );
    }
}

// This will do an atomic copy of the object
void LogFilteredDataWorkerThread::getSearchResult(
        int* maxLength, SearchResultArray* searchMatches, qint64* nbLinesProcessed )
{
    searchData_.getAll( maxLength, searchMatches, nbLinesProcessed );
}

// This is the thread's main loop
void LogFilteredDataWorkerThread::run()
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
            connect( operationRequested_, SIGNAL( searchProgressed( int, int, qint64 ) ),
                    this, SIGNAL( searchProgressed( int, int, qint64 ) ) );

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
        const QRegularExpression& regExp, bool* interruptRequest )
    : regexp_( regExp ), sourceLogData_( sourceLogData )
{
    interruptRequested_ = interruptRequest;
}

void SearchOperation::doSearch( SearchData& searchData, qint64 initialLine )
{
    const qint64 nbSourceLines = sourceLogData_->getNbLine();
    int maxLength = 0;
    int nbMatches = searchData.getNbMatches();
    SearchResultArray currentList = SearchResultArray();

    // Ensure no re-alloc will be done
    currentList.reserve( nbLinesInChunk );

    LOG(logDEBUG) << "Searching from line " << initialLine << " to " << nbSourceLines;

    for ( qint64 i = initialLine; i < nbSourceLines; i += nbLinesInChunk ) {
        if ( *interruptRequested_ )
            break;

        const int percentage = ( i - initialLine ) * 100 / ( nbSourceLines - initialLine );
        emit searchProgressed( nbMatches, percentage, initialLine );

        const QStringList lines = sourceLogData_->getLines( i,
                qMin( nbLinesInChunk, (int) ( nbSourceLines - i ) ) );
        LOG(logDEBUG) << "Chunk starting at " << i <<
            ", " << lines.size() << " lines read.";

        int j = 0;
        for ( ; j < lines.size(); j++ ) {
            if ( regexp_.match( lines[j] ).hasMatch() ) {
                // FIXME: increase perf by removing temporary
                const int length = sourceLogData_->getExpandedLineString(i+j).length();
                if ( length > maxLength )
                    maxLength = length;
                currentList.push_back( MatchingLine( i+j ) );
                nbMatches++;
            }
        }

        // After each block, copy the data to shared data
        // and update the client
        searchData.addAll( maxLength, currentList, i+j );
        currentList.clear();
    }

    emit searchProgressed( nbMatches, 100, initialLine );
}

// Called in the worker thread's context
void FullSearchOperation::start( SearchData& searchData )
{
    // Clear the shared data
    searchData.clear();

    doSearch( searchData, 0 );
}

// Called in the worker thread's context
void UpdateSearchOperation::start( SearchData& searchData )
{
    qint64 initial_line = initialPosition_;

    if ( initial_line >= 1 ) {
        // We need to re-search the last line because it might have
        // been updated (if it was not LF-terminated)
        --initial_line;
        // In case the last line matched, we don't want it to match twice.
        searchData.deleteMatch( initial_line );
    }

    doSearch( searchData, initial_line );
}
