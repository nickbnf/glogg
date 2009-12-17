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

#include "logfiltereddataworkerthread.h"
#include "logdata.h"
#include "common.h"

void SearchData::getAll( int* length, SearchResultArray* matches )
{
    QMutexLocker locker( &dataMutex_ );

    *length  = maxLength_;
    *matches = matches_;
}

void SearchData::setAll( int length,
        const SearchResultArray& matches )
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_  = length;
    matches_    = matches;
}

void SearchData::addAll( int length,
        const SearchResultArray& matches )
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_  = max2( maxLength_, length );
    matches_   += matches;
}

SearchOperation::SearchOperation( const QRegExp& regExp,
        bool* interruptRequest ) : regexp_( regExp )
{
    interruptRequest_ = interruptRequest;
}

const QRegExp& SearchOperation::regExp() const
{
    return regexp_;
}


LogFilteredDataWorkerThread::LogFilteredDataWorkerThread(
        const LogData* sourceLogData )
    : QThread(), mutex_(), operationRequestedCond_(), searchData_()
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

void LogFilteredDataWorkerThread::search( const QRegExp& regExp )
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "Search requested";
    interruptRequested_ = false;
    operationRequested_ = new SearchOperation( regExp, &interruptRequested_ );
    operationRequestedCond_.wakeAll();

    // Make it blocking until operation started.
}

void LogFilteredDataWorkerThread::interrupt()
{
    // QMutexLocker locker( &mutex_ );  // to protect interruptRequested_

    LOG(logDEBUG) << "Interrupt requested";
    interruptRequested_ = true;

    // Make it blocking until operation cancelled.
}

// This will do an atomic copy of the object
// (hopefully fast as we use Qt containers)
void LogFilteredDataWorkerThread::getSearchResult(
        int* maxLength, SearchResultArray* searchMatches )
{
    searchData_.getAll( maxLength, searchMatches );
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
            // Run the search operation
            doSearch( operationRequested_ );

            LOG(logDEBUG) << "... finished copy in workerThread.";

            emit searchFinished();
            delete operationRequested_;
            operationRequested_ = NULL;
        }
    }
}

//
// Operations implementation
//

// Called in the worker thread's context
void LogFilteredDataWorkerThread::doSearch( const SearchOperation* searchOperation )
{
    const QRegExp regExp = searchOperation->regExp();
    int maxLength = 0, nbMatches = 0;
    SearchResultArray currentList = SearchResultArray();

    for ( int i = 0; i < sourceLogData_->getNbLine(); i++ ) {
        const QString line = sourceLogData_->getLineString( i );
        if ( regExp.indexIn( line ) != -1 ) {
            const int length = line.length();
            if ( length > maxLength )
                maxLength = length;
            MatchingLine match( i, line );
            currentList.append( match );
            nbMatches++;
        }

        // Every once in a while, copy the data to shared data
        // and update the client
        if ( !( i % 5000 ) ) {
            searchData_.addAll( maxLength, currentList );
            currentList.clear();

            emit searchProgressed( nbMatches,
                    i * 100 / sourceLogData_->getNbLine() );
        }
    }

    searchData_.addAll( maxLength, currentList );
    emit searchProgressed( nbMatches, 100 );
}
