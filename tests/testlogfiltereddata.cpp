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

void TestLogFilteredData::initTestCase()
{
    QVERIFY( generateDataFiles() );
}

void TestLogFilteredData::simpleSearch()
{
    LogData logData;

    // First load the tests file
    // Register for notification file is loaded
    connect( &logData, SIGNAL( loadingFinished( bool ) ),
            this, SLOT( loadingFinished() ) );

    logData.attachFile( TMPDIR "/mediumlog.txt" );
    // Wait for the loading to be done
    {
        QApplication::exec();
    }

    QCOMPARE( logData.getNbLine(), ML_NB_LINES );

    // Now perform a simple search
    LogFilteredData* filteredData = logData.getNewFilteredData();
    connect( filteredData, SIGNAL( searchProgressed( int, int ) ),
            this, SLOT( searchProgressed( int, int ) ) );

    QSignalSpy progressSpy( filteredData, SIGNAL( searchProgressed( int, int ) ) );

    qint64 matches[] = { 0, 15, 20, 135 };
    QBENCHMARK {
        // Start the search
        filteredData->runSearch( QRegExp( "123" ) );

        // And check we receive data in 4 chunks (the first being empty)
        for ( int i = 0; i < 4; i++ ) {
            QApplication::exec();
            QCOMPARE( filteredData->getNbLine(), matches[i] );
        }
    }

    QCOMPARE( progressSpy.count(), 4 );

    // Check the search
    QCOMPARE( filteredData->isLineInMatchingList( 123 ), true );
    QCOMPARE( filteredData->isLineInMatchingList( 124 ), false );
    QCOMPARE( filteredData->getMaxLength(), ML_VISIBLE_LINE_LENGTH );
    QCOMPARE( filteredData->getLineLength( 12 ), ML_VISIBLE_LINE_LENGTH );
    QCOMPARE( filteredData->getNbLine(), 135LL );
    // Line beyond limit
    QCOMPARE( filteredData->isLineInMatchingList( 60000 ), false );
    QCOMPARE( filteredData->getMatchingLineNumber( 0 ), 123LL );

    // Now let's try interrupting a search
    filteredData->runSearch( QRegExp( "123" ) );
    // ... wait for two chunks.
    QApplication::exec();
    QApplication::exec();
    // and interrupt!
    filteredData->interruptSearch();
    QCOMPARE( filteredData->getNbLine(), matches[1] );
    QApplication::exec();
    // After interrupt: should be 100% and the same number of matches
    QCOMPARE( filteredData->getNbLine(), matches[2] );

    // (because there is no guarantee when the search is 
    // interrupted, we are not sure how many chunk of result
    // we will get.)

    // Disconnect all signals
    disconnect( &logData, 0 );

    // Destroy the filtered data
    delete filteredData;
}

void TestLogFilteredData::multipleSearch()
{
    LogData logData;

    // First load the tests file
    // Register for notification file is loaded
    connect( &logData, SIGNAL( loadingFinished( bool ) ),
            this, SLOT( loadingFinished() ) );

    logData.attachFile( TMPDIR "/smalllog.txt" );
    // Wait for the loading to be done
    {
        QApplication::exec();
    }

    QCOMPARE( logData.getNbLine(), SL_NB_LINES );

    // Performs two searches in a row
    LogFilteredData* filteredData = logData.getNewFilteredData();
    connect( filteredData, SIGNAL( searchProgressed( int, int ) ),
            this, SLOT( searchProgressed( int, int ) ) );

    QSignalSpy progressSpy( filteredData,
            SIGNAL( searchProgressed( int, int ) ) );

    // Start the search, and immediately another one
    // (the second call should block until the first search is done)
    filteredData->runSearch( QRegExp( "1234" ) );
    filteredData->runSearch( QRegExp( "123" ) );

    for ( int i = 0; i < 3; i++ )
        QApplication::exec();

    // We should have the result for the 2nd search
    QCOMPARE( filteredData->getNbLine(), 12LL );

    QCOMPARE( progressSpy.count(), 4 );

    // Now a tricky one: we run a search and immediately attach a new file
    filteredData->runSearch( QRegExp( "123" ) );
    QApplication::exec();
    logData.attachFile( TMPDIR "/mediumlog.txt" );

    // We don't expect meaningful results but it should not crash!
    for ( int i = 0; i < 2; i++ )
        QApplication::exec();

    // Disconnect all signals
    disconnect( &logData, 0 );

    // Destroy the filtered data
    delete filteredData;
}

void TestLogFilteredData::updateSearch()
{
    LogData logData;

    // First load the tests file
    // Register for notification file is loaded
    connect( &logData, SIGNAL( loadingFinished( bool ) ),
            this, SLOT( loadingFinished() ) );

    logData.attachFile( TMPDIR "/smalllog.txt" );
    // Wait for the loading to be done
    {
        QApplication::exec();
    }

    QCOMPARE( logData.getNbLine(), SL_NB_LINES );

    // Perform a first search
    LogFilteredData* filteredData = logData.getNewFilteredData();
    connect( filteredData, SIGNAL( searchProgressed( int, int ) ),
            this, SLOT( searchProgressed( int, int ) ) );

    QSignalSpy progressSpy( filteredData,
            SIGNAL( searchProgressed( int, int ) ) );

    // Start a search
    filteredData->runSearch( QRegExp( "123" ) );

    for ( int i = 0; i < 2; i++ )
        QApplication::exec();

    // Check the result
    QCOMPARE( filteredData->getNbLine(), 12LL );
    QCOMPARE( progressSpy.count(), 2 );

    // Add some data to the file
    char newLine[90];
    QFile file( TMPDIR "/smalllog.txt" );
    if ( file.open( QIODevice::Append ) ) {
        for (int i = 0; i < 3000; i++) {
            snprintf(newLine, 89, sl_format, i);
            file.write( newLine, qstrlen(newLine) );
        }
        // To test the edge case when the final line is not complete
        snprintf(newLine, 89, "123... beginning of line.");
        file.write( newLine, qstrlen(newLine) );
    }
    file.close();

    // Let the system do the update
    QApplication::exec();

    // Start an update search
    filteredData->updateSearch();

    for ( int i = 0; i < 2; i++ )
        QApplication::exec();

    // Check the result
    QCOMPARE( logData.getNbLine(), 5001LL );
    QCOMPARE( filteredData->getNbLine(), 26LL );
    QCOMPARE( progressSpy.count(), 4 );
}

//
// Private functions
//
void TestLogFilteredData::loadingFinished()
{
    QApplication::quit();
}

void TestLogFilteredData::searchProgressed( int nbMatches, int completion )
{
    QApplication::quit();
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
