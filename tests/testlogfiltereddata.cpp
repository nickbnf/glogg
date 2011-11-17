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

#include <QSignalSpy>
#include <QMutexLocker>
#include <QFile>

#include "testlogfiltereddata.h"
#include "logdata.h"
#include "logfiltereddata.h"

#if QT_VERSION < 0x040500
#define QBENCHMARK
#endif

#if !defined( TMPDIR )
#define TMPDIR "/tmp"
#endif

static const qint64 ML_NB_LINES = 15000LL;
static const char* ml_format="LOGDATA is a part of glogg, we are going to test it thoroughly, this is line\t\t%06d\n";
static const int ML_VISIBLE_LINE_LENGTH = (76+8+4+6); // Without the final '\n' !

static const qint64 SL_NB_LINES = 2000LL;
static const char* sl_format="LOGDATA is a part of glogg, we are going to test it thoroughly, this is line %06d\n";

static const char* partial_line_begin = "123... beginning of line.";
static const char* partial_line_end = " end of line 123.\n";

static const char* partial_nonmatching_line_begin = "Beginning of line.";

TestLogFilteredData::TestLogFilteredData() : QObject(),
    loadingFinishedMutex_(),
    searchProgressedMutex_(),
    loadingFinishedCondition_(),
    searchProgressedCondition_()
{
    loadingFinished_received_  = false;
    loadingFinished_read_      = false;
    searchProgressed_received_ = false;
    searchProgressed_read_     = false;
}

void TestLogFilteredData::initTestCase()
{
    QVERIFY( generateDataFiles() );
}

void TestLogFilteredData::simpleSearch()
{
    logData_ = new LogData();

    // Register for notification file is loaded
    connect( logData_, SIGNAL( loadingFinished( bool ) ),
            this, SLOT( loadingFinished() ) );

    filteredData_ = logData_->getNewFilteredData();
    connect( filteredData_, SIGNAL( searchProgressed( int, int ) ),
            this, SLOT( searchProgressed( int, int ) ) );

    QFuture<void> future = QtConcurrent::run(this, &TestLogFilteredData::simpleSearchTest);

    QApplication::exec();

    disconnect( filteredData_, 0 );
    disconnect( logData_, 0 );

    delete filteredData_;
    delete logData_;
}

void TestLogFilteredData::simpleSearchTest()
{
    // First load the tests file
    logData_->attachFile( TMPDIR "/mediumlog.txt" );
    // Wait for the loading to be done
    waitLoadingFinished();
    QCOMPARE( logData_->getNbLine(), ML_NB_LINES );
    signalLoadingFinishedRead();

    // Now perform a simple search
    qint64 matches[] = { 0, 15, 20, 135 };
    QBENCHMARK {
        // Start the search
        filteredData_->runSearch( QRegExp( "123" ) );

        // And check we receive data in 4 chunks (the first being empty)
        for ( int i = 0; i < 4; i++ ) {
            std::pair<int,int> progress = waitSearchProgressed();
            QCOMPARE( (qint64) progress.first, matches[i] );
            signalSearchProgressedRead();
        }
    }

    QCOMPARE( filteredData_->getNbLine(), matches[3] );
    // Check the search
    QCOMPARE( filteredData_->isLineInMatchingList( 123 ), true );
    QCOMPARE( filteredData_->isLineInMatchingList( 124 ), false );
    QCOMPARE( filteredData_->getMaxLength(), ML_VISIBLE_LINE_LENGTH );
    QCOMPARE( filteredData_->getLineLength( 12 ), ML_VISIBLE_LINE_LENGTH );
    QCOMPARE( filteredData_->getNbLine(), 135LL );
    // Line beyond limit
    QCOMPARE( filteredData_->isLineInMatchingList( 60000 ), false );
    QCOMPARE( filteredData_->getMatchingLineNumber( 0 ), 123LL );

    // Now let's try interrupting a search
    filteredData_->runSearch( QRegExp( "123" ) );
    // ... wait for two chunks.
    waitSearchProgressed();
    signalSearchProgressedRead();
    waitSearchProgressed();
    // and interrupt!
    filteredData_->interruptSearch();
    QCOMPARE( filteredData_->getNbLine(), matches[1] );
    waitSearchProgressed();
    // After interrupt: should be 100% and the same number of matches
    QCOMPARE( filteredData_->getNbLine(), matches[1] );
    signalSearchProgressedRead();

    // (because there is no guarantee when the search is 
    // interrupted, we are not sure how many chunk of result
    // we will get.)

    QApplication::quit();
}

void TestLogFilteredData::multipleSearch()
{
    logData_ = new LogData();

    // Register for notification file is loaded
    connect( logData_, SIGNAL( loadingFinished( bool ) ),
            this, SLOT( loadingFinished() ) );

    filteredData_ = logData_->getNewFilteredData();
    connect( filteredData_, SIGNAL( searchProgressed( int, int ) ),
            this, SLOT( searchProgressed( int, int ) ) );

    QFuture<void> future = QtConcurrent::run(this, &TestLogFilteredData::multipleSearchTest);

    QApplication::exec();

    disconnect( filteredData_, 0 );
    disconnect( logData_, 0 );

    delete filteredData_;
    delete logData_;
}

void TestLogFilteredData::multipleSearchTest()
{
    // First load the tests file
    logData_->attachFile( TMPDIR "/smalllog.txt" );
    // Wait for the loading to be done
    waitLoadingFinished();
    QCOMPARE( logData_->getNbLine(), SL_NB_LINES );
    signalLoadingFinishedRead();

    // Performs two searches in a row
    QSignalSpy progressSpy( filteredData_,
            SIGNAL( searchProgressed( int, int ) ) );

    // Start the search, and immediately another one
    // (the second call should block until the first search is done)
    filteredData_->runSearch( QRegExp( "1234" ) );
    filteredData_->runSearch( QRegExp( "123" ) );

    for ( int i = 0; i < 3; i++ ) {
        waitSearchProgressed();
        signalSearchProgressedRead();
    }

    // We should have the result for the 2nd search after the last chunk
    waitSearchProgressed();
    QCOMPARE( filteredData_->getNbLine(), 12LL );
    signalSearchProgressedRead();

    // Now a tricky one: we run a search and immediately attach a new file
    filteredData_->runSearch( QRegExp( "123" ) );
    waitSearchProgressed();
    signalSearchProgressedRead();
    logData_->attachFile( TMPDIR "/mediumlog.txt" );

    // We don't expect meaningful results but it should not crash!
    for ( int i = 0; i < 1; i++ ) {
        waitSearchProgressed();
        signalSearchProgressedRead();
    }

    QApplication::quit();
}

void TestLogFilteredData::updateSearch()
{
    logData_ = new LogData();

    // Register for notification file is loaded
    connect( logData_, SIGNAL( loadingFinished( bool ) ),
            this, SLOT( loadingFinished() ) );

    filteredData_ = logData_->getNewFilteredData();
    connect( filteredData_, SIGNAL( searchProgressed( int, int ) ),
            this, SLOT( searchProgressed( int, int ) ) );

    QFuture<void> future = QtConcurrent::run(this, &TestLogFilteredData::updateSearchTest);

    QApplication::exec();

    disconnect( filteredData_, 0 );
    disconnect( logData_, 0 );

    delete filteredData_;
    delete logData_;
}

void TestLogFilteredData::updateSearchTest()
{
    // First load the tests file
    logData_->attachFile( TMPDIR "/smalllog.txt" );
    // Wait for the loading to be done
    waitLoadingFinished();
    QCOMPARE( logData_->getNbLine(), SL_NB_LINES );
    signalLoadingFinishedRead();

    // Perform a first search
    filteredData_->runSearch( QRegExp( "123" ) );

    for ( int i = 0; i < 2; i++ ) {
        waitSearchProgressed();
        signalSearchProgressedRead();
    }

    // Check the result
    QCOMPARE( filteredData_->getNbLine(), 12LL );

    QWARN("Starting stage 2");

    // Add some data to the file
    char newLine[90];
    QFile file( TMPDIR "/smalllog.txt" );
    if ( file.open( QIODevice::Append ) ) {
        for (int i = 0; i < 3000; i++) {
            snprintf(newLine, 89, sl_format, i);
            file.write( newLine, qstrlen(newLine) );
        }
        // To test the edge case when the final line is not complete and matching
        file.write( partial_line_begin, qstrlen( partial_line_begin ) );
    }
    file.close();

    // Let the system do the update
    waitLoadingFinished();
    signalLoadingFinishedRead();

    // Start an update search
    filteredData_->updateSearch();

    for ( int i = 0; i < 2; i++ ) {
        waitSearchProgressed();
        signalSearchProgressedRead();
    }

    // Check the result
    QCOMPARE( logData_->getNbLine(), 5001LL );
    QCOMPARE( filteredData_->getNbLine(), 26LL );

    QWARN("Starting stage 3");

    // Add a couple more lines, including the end of the unfinished one.
    if ( file.open( QIODevice::Append ) ) {
        file.write( partial_line_end, qstrlen( partial_line_end ) );
        for (int i = 0; i < 20; i++) {
            snprintf(newLine, 89, sl_format, i);
            file.write( newLine, qstrlen(newLine) );
        }
        // To test the edge case when the final line is not complete and not matching
        file.write( partial_nonmatching_line_begin,
                qstrlen( partial_nonmatching_line_begin ) );
    }
    file.close();

    // Let the system do the update
    waitLoadingFinished();
    signalLoadingFinishedRead();

    // Start an update search
    filteredData_->updateSearch();

    for ( int i = 0; i < 2; i++ ) {
        waitSearchProgressed();
        signalSearchProgressedRead();
    }

    // Check the result
    QCOMPARE( logData_->getNbLine(), 5022LL );
    QCOMPARE( filteredData_->getNbLine(), 26LL );

    QWARN("Starting stage 4");

    // Now test the case where a match is found at the end of an updated line.
    if ( file.open( QIODevice::Append ) ) {
        file.write( partial_line_end, qstrlen( partial_line_end ) );
        for (int i = 0; i < 20; i++) {
            snprintf(newLine, 89, sl_format, i);
            file.write( newLine, qstrlen(newLine) );
        }
    }
    file.close();

    // Let the system do the update
    waitLoadingFinished();
    signalLoadingFinishedRead();

    // Start an update search
    filteredData_->updateSearch();

    for ( int i = 0; i < 2; i++ ) {
        waitSearchProgressed();
        signalSearchProgressedRead();
    }

    // Check the result
    QCOMPARE( logData_->getNbLine(), 5042LL );
    QCOMPARE( filteredData_->getNbLine(), 27LL );

    QApplication::quit();
}

//
// Private functions
//
void TestLogFilteredData::loadingFinished()
{
    QMutexLocker locker( &loadingFinishedMutex_ );

    QWARN("loadingFinished");
    loadingFinished_received_ = true;
    loadingFinished_read_ = false;

    loadingFinishedCondition_.wakeOne();

    // Wait for the test thread to read the signal
    while ( ! loadingFinished_read_ )
        loadingFinishedCondition_.wait( locker.mutex() );
}

void TestLogFilteredData::searchProgressed( int nbMatches, int completion )
{
    QMutexLocker locker( &searchProgressedMutex_ );

    QWARN("searchProgressed");
    searchProgressed_received_ = true;
    searchProgressed_read_ = false;

    searchLastMatches_ = nbMatches;
    searchLastProgress_ = completion;

    searchProgressedCondition_.wakeOne();

    // Wait for the test thread to read the signal
    while ( ! searchProgressed_read_ )
        searchProgressedCondition_.wait( locker.mutex() );
}

std::pair<int,int> TestLogFilteredData::waitSearchProgressed()
{
    QMutexLocker locker( &searchProgressedMutex_ );

    while ( ! searchProgressed_received_ )
        searchProgressedCondition_.wait( locker.mutex() );

    QWARN("searchProgressed Received");

    return std::pair<int,int>(searchLastMatches_, searchLastProgress_);
}

void TestLogFilteredData::waitLoadingFinished()
{
    QMutexLocker locker( &loadingFinishedMutex_ );

    while ( ! loadingFinished_received_ )
        loadingFinishedCondition_.wait( locker.mutex() );

    QWARN("loadingFinished Received");
}

void TestLogFilteredData::signalSearchProgressedRead()
{
    QMutexLocker locker( &searchProgressedMutex_ );

    searchProgressed_received_ = false;
    searchProgressed_read_ = true;
    searchProgressedCondition_.wakeOne();
}

void TestLogFilteredData::signalLoadingFinishedRead()
{
    QMutexLocker locker( &loadingFinishedMutex_ );

    loadingFinished_received_ = false;
    loadingFinished_read_ = true;
    loadingFinishedCondition_.wakeOne();
}

bool TestLogFilteredData::generateDataFiles()
{
    char newLine[90];

    QFile file( TMPDIR "/mediumlog.txt" );
    if ( file.open( QIODevice::WriteOnly ) ) {
        for (int i = 0; i < ML_NB_LINES; i++) {
            snprintf(newLine, 89, ml_format, i);
            file.write( newLine, qstrlen(newLine) );
        }
    }
    else {
        return false;
    }
    file.close();

    file.setFileName( TMPDIR "/smalllog.txt" );
    if ( file.open( QIODevice::WriteOnly ) ) {
        for (int i = 0; i < SL_NB_LINES; i++) {
            snprintf(newLine, 89, sl_format, i);
            file.write( newLine, qstrlen(newLine) );
        }
    }
    else {
        return false;
    }
    file.close();

    return true;
}
