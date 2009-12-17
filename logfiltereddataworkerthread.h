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

#ifndef LOGFILTEREDDATAWORKERTHREAD_H
#define LOGFILTEREDDATAWORKERTHREAD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QRegExp>
#include <QList>

// #include "logdata.h"

class LogData;

// Class encapsulating a single matching line
// Contains the line number the line was found in and its content.
class MatchingLine {
  public:
    MatchingLine( int line, QString str ) { lineNumber_ = line; lineString_ = str; };

    // Accessors
    int lineNumber() const { return lineNumber_; }
    QString lineContent() const { return lineString_; }

  private:
    int lineNumber_;
    QString lineString_;
};

typedef QList<MatchingLine> SearchResultArray;

// This class is a mutex protected set of search result data.
// It is thread safe.
class SearchData
{
  public:
    SearchData() : dataMutex_(), matches_(), maxLength_(0) { }

    // Atomically get all the search data
    void getAll( int* length, SearchResultArray* matches );

    // Atomically set all the search data
    // (overwriting the existing)
    void setAll( int length, const SearchResultArray& matches );

    // Atomically add to all the existing search data.
    void addAll( int length, const SearchResultArray& matches );

  private:
    QMutex dataMutex_;

    SearchResultArray matches_;
    int maxLength_;
};

class SearchOperation : public QObject
{
  public:
    SearchOperation( const QRegExp& regExp, bool* interruptRequest );

    const QRegExp& regExp() const;

  private:
    QRegExp regexp_;
    bool* interruptRequest_;
};

// Create and manage the thread doing loading/indexing for
// the creating LogData. One LogDataWorkerThread is used
// per LogData instance.
// Note everything except the run() function is in the LogData's
// thread.
class LogFilteredDataWorkerThread : public QThread
{
  Q_OBJECT

  public:
    LogFilteredDataWorkerThread( const LogData* sourceLogData );
    ~LogFilteredDataWorkerThread();

    void search( const QRegExp& regExp );
    // Interrupts the search if one is in progress
    void interrupt();

    // Returns a copy of the current indexing data
    void getSearchResult( int* maxLength, SearchResultArray* searchMatches );

  signals:
    // Sent during the indexing process to signal progress
    // percent being the percentage of completion.
    void searchProgressed( int percent, int nbMatches );
    // Sent when indexing is finished, signals the client
    // to copy the new data back.
    void searchFinished();

  protected:
    void run();

  private:
    void doSearch( const SearchOperation* searchOperation );

    const LogData* sourceLogData_;

    // Mutex to protect operationRequested_ and friends
    QMutex mutex_;
    QWaitCondition operationRequestedCond_;

    // Set when the thread must die
    bool terminate_;
    bool interruptRequested_;
    SearchOperation* operationRequested_;

    // Shared indexing data
    SearchData searchData_;
};

#endif
