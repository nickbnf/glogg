/*
 * Copyright (C) 2009, 2010, 2011 Nicolas Bonnefon and other contributors
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
        filteredData_->runSearch( QRegularExpression( "123" ) );

        // And check we receive data in 4 chunks (the first being empty)
        for ( int i = 0; i < 4; i++ ) {
            std::pair<int,int> progress = waitSearchProgressed();
            // FIXME: The test for this is unfortunately not reliable
            // (race conditions)
            // QCOMPARE( (qint64) progress.first, matches[i] );
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
    filteredData_->runSearch( QRegularExpression( "123" ) );
    // ... wait for two chunks.
    waitSearchProgressed();
    signalSearchProgressedRead();
    // and interrupt!
    filteredData_->interruptSearch();

    {
        std::pair<int,int> progress;
        do {
            progress = waitSearchProgressed();
            signalSearchProgressedRead();
        } while ( progress.second < 100 );

        // (because there is no guarantee when the search is
        // interrupted, we are not sure how many chunk of result
        // we will get.)
    }

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
    // Start the search, and immediately another one
    // (the second call should block until the first search is done)
    filteredData_->runSearch( QRegularExpression( "1234" ) );
    filteredData_->runSearch( QRegularExpression( "123" ) );

    for ( int i = 0; i < 3; i++ ) {
        waitSearchProgressed();
        signalSearchProgressedRead();
    }

    // We should have the result for the 2nd search after the last chunk
    waitSearchProgressed();
    QCOMPARE( filteredData_->getNbLine(), 12LL );
    signalSearchProgressedRead();

    // Now a tricky one: we run a search and immediately attach a new file
    /* FIXME: sometimes we receive loadingFinished before searchProgressed
     * -> deadlock in the test.
    filteredData_->runSearch( QRegularExpression( "123" ) );
    waitSearchProgressed();
    signalSearchProgressedRead();
    logData_->attachFile( TMPDIR "/mediumlog.txt" );

    // We don't expect meaningful results but it should not crash!
    for ( int i = 0; i < 1; i++ ) {
        waitSearchProgressed();
        signalSearchProgressedRead();
    }
    */

    sleep(10);

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
    filteredData_->runSearch( QRegularExpression( "123" ) );

    for ( int i = 0; i < 2; i++ ) {
        waitSearchProgressed();
        signalSearchProgressedRead();
    }

    // Check the result
    QCOMPARE( filteredData_->getNbLine(), 12LL );

    sleep(1);

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

    // Let the system do the update (there might be several ones)
    do {
        waitLoadingFinished();
        signalLoadingFinishedRead();
    } while ( logData_->getNbLine() < 5001LL );

    sleep(1);

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

void TestLogFilteredData::marks()
{
    logData_ = new LogData();

    // Register for notification file is loaded
    connect( logData_, SIGNAL( loadingFinished( bool ) ),
            this, SLOT( loadingFinished() ) );

    filteredData_ = logData_->getNewFilteredData();
    connect( filteredData_, SIGNAL( searchProgressed( int, int ) ),
            this, SLOT( searchProgressed( int, int ) ) );

    QFuture<void> future = QtConcurrent::run(this, &TestLogFilteredData::marksTest);

    QApplication::exec();

    disconnect( filteredData_, 0 );
    disconnect( logData_, 0 );

    delete filteredData_;
    delete logData_;
}

void TestLogFilteredData::marksTest()
{
    // First load the tests file
    logData_->attachFile( TMPDIR "/smalllog.txt" );
    // Wait for the loading to be done
    waitLoadingFinished();
    QCOMPARE( logData_->getNbLine(), SL_NB_LINES );
    signalLoadingFinishedRead();

    // First check no line is marked
    for ( int i = 0; i < SL_NB_LINES; i++ )
        QVERIFY( filteredData_->isLineMarked( i ) == false );

    // Try to create some "out of limit" marks
    filteredData_->addMark( -10 );
    filteredData_->addMark( SL_NB_LINES + 25 );

    // Check no line is marked still
    for ( int i = 0; i < SL_NB_LINES; i++ )
        QVERIFY( filteredData_->isLineMarked( i ) == false );

    // Create a couple of unnamed marks
    filteredData_->addMark( 10 );
    filteredData_->addMark( 44 );  // This one will also be a match
    filteredData_->addMark( 25 );

    // Check they are marked
    QVERIFY( filteredData_->isLineMarked( 10 ) );
    QVERIFY( filteredData_->isLineMarked( 25 ) );
    QVERIFY( filteredData_->isLineMarked( 44 ) );

    // But others are not
    QVERIFY( filteredData_->isLineMarked( 15 ) == false );
    QVERIFY( filteredData_->isLineMarked( 20 ) == false );

    QCOMPARE( filteredData_->getNbLine(), 3LL );

    // Performs a search
    QSignalSpy progressSpy( filteredData_,
            SIGNAL( searchProgressed( int, int ) ) );
    filteredData_->runSearch( QRegularExpression( "0000.4" ) );

    for ( int i = 0; i < 1; i++ ) {
        waitSearchProgressed();
        signalSearchProgressedRead();
    }

    // We should have the result of the search and the marks
    waitSearchProgressed();
    QCOMPARE( filteredData_->getNbLine(), 12LL );
    signalSearchProgressedRead();

    QString startline = "LOGDATA is a part of glogg, we are going to test it thoroughly, this is line ";

    QCOMPARE( filteredData_->getLineString(0), startline + "000004" );
    QCOMPARE( filteredData_->getLineString(1), startline + "000010" );
    QCOMPARE( filteredData_->getLineString(2), startline + "000014" );
    QCOMPARE( filteredData_->getLineString(3), startline + "000024" );
    QCOMPARE( filteredData_->getLineString(4), startline + "000025" );
    QCOMPARE( filteredData_->getLineString(5), startline + "000034" );
    QCOMPARE( filteredData_->getLineString(6), startline + "000044" );
    QCOMPARE( filteredData_->getLineString(7), startline + "000054" );
    QCOMPARE( filteredData_->getLineString(8), startline + "000064" );
    QCOMPARE( filteredData_->getLineString(9), startline + "000074" );
    QCOMPARE( filteredData_->getLineString(10), startline + "000084" );
    QCOMPARE( filteredData_->getLineString(11), startline + "000094" );

    filteredData_->setVisibility( LogFilteredData::MatchesOnly );

    QCOMPARE( filteredData_->getNbLine(), 10LL );
    QCOMPARE( filteredData_->getLineString(0), startline + "000004" );
    QCOMPARE( filteredData_->getLineString(1), startline + "000014" );
    QCOMPARE( filteredData_->getLineString(2), startline + "000024" );
    QCOMPARE( filteredData_->getLineString(3), startline + "000034" );
    QCOMPARE( filteredData_->getLineString(4), startline + "000044" );
    QCOMPARE( filteredData_->getLineString(5), startline + "000054" );
    QCOMPARE( filteredData_->getLineString(6), startline + "000064" );
    QCOMPARE( filteredData_->getLineString(7), startline + "000074" );
    QCOMPARE( filteredData_->getLineString(8), startline + "000084" );
    QCOMPARE( filteredData_->getLineString(9), startline + "000094" );

    filteredData_->setVisibility( LogFilteredData::MarksOnly );

    QCOMPARE( filteredData_->getNbLine(), 3LL );
    QCOMPARE( filteredData_->getLineString(0), startline + "000010" );
    QCOMPARE( filteredData_->getLineString(1), startline + "000025" );
    QCOMPARE( filteredData_->getLineString(2), startline + "000044" );

    // Another test with marks only
    filteredData_->clearSearch();
    filteredData_->clearMarks();
    filteredData_->setVisibility( LogFilteredData::MarksOnly );

    filteredData_->addMark(18);
    filteredData_->addMark(19);
    filteredData_->addMark(20);

    QCOMPARE( filteredData_->getMatchingLineNumber(0), 18LL );
    QCOMPARE( filteredData_->getMatchingLineNumber(1), 19LL );
    QCOMPARE( filteredData_->getMatchingLineNumber(2), 20LL );

    QApplication::quit();
}

void TestLogFilteredData::lineLength()
{
    logData_ = new LogData();

    // Register for notification file is loaded
    connect( logData_, SIGNAL( loadingFinished( bool ) ),
            this, SLOT( loadingFinished() ) );

    filteredData_ = logData_->getNewFilteredData();
    connect( filteredData_, SIGNAL( searchProgressed( int, int ) ),
            this, SLOT( searchProgressed( int, int ) ) );

    QFuture<void> future = QtConcurrent::run(this, &TestLogFilteredData::lineLengthTest);

    QApplication::exec();

    disconnect( filteredData_, 0 );
    disconnect( logData_, 0 );

    delete filteredData_;
    delete logData_;
}

void TestLogFilteredData::lineLengthTest()
{
    // Line length tests

    logData_->attachFile( TMPDIR "/length_test.txt" );
    // Wait for the loading to be done
    waitLoadingFinished();
    QCOMPARE( logData_->getNbLine(), 4LL );
    signalLoadingFinishedRead();

    // Performs a search (the two middle lines matche)
    filteredData_->setVisibility( LogFilteredData::MatchesOnly );
    filteredData_->runSearch( QRegularExpression( "longer" ) );

    std::pair<int,int> progress;
    do {
        progress = waitSearchProgressed();
        signalSearchProgressedRead();
        QWARN("progress");
    } while ( progress.second < 100 );

    filteredData_->addMark( 3 );

    QCOMPARE( filteredData_->getNbLine(), 2LL );
    QCOMPARE( filteredData_->getMaxLength(), 40 );

    filteredData_->setVisibility( LogFilteredData::MarksAndMatches );
    QCOMPARE( filteredData_->getNbLine(), 3LL );
    QCOMPARE( filteredData_->getMaxLength(), 103 );

    filteredData_->setVisibility( LogFilteredData::MarksOnly );
    QCOMPARE( filteredData_->getNbLine(), 1LL );
    QCOMPARE( filteredData_->getMaxLength(), 103 );

    filteredData_->addMark( 0 );
    QCOMPARE( filteredData_->getNbLine(), 2LL );
    QCOMPARE( filteredData_->getMaxLength(), 103 );
    filteredData_->deleteMark( 3 );
    QCOMPARE( filteredData_->getNbLine(), 1LL );
    QCOMPARE( filteredData_->getMaxLength(), 27 );

    filteredData_->setVisibility( LogFilteredData::MarksAndMatches );
    QCOMPARE( filteredData_->getMaxLength(), 40 );

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

    file.setFileName( TMPDIR "/length_test.txt" );
    if ( file.open( QIODevice::WriteOnly ) ) {
        file.write( "This line is 27 characters.\n" );
        file.write( "This line is longer: 36 characters.\n" );
        file.write( "This line is even longer: 40 characters.\n" );
        file.write( "This line is very long, it's actually hard to count but it is\
 probably something around 103 characters.\n" );
    }
    file.close();

    return true;
}
