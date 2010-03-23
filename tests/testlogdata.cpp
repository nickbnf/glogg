#include <QSignalSpy>
#include <QMutexLocker>
#include <QFile>

#include "testlogdata.h"
#include "logdata.h"

#if QT_VERSION < 0x040500
#define QBENCHMARK
#endif

#if !defined( TMPDIR )
#define TMPDIR "/tmp"
#endif

static const int VBL_NB_LINES = 4999999;
static const int VBL_LINE_PER_PAGE = 70;
static const char* vbl_format="LOGDATA is a part of glogg, we are going to test it thoroughly, this is line %07d\n";
static const int VBL_LINE_LENGTH = 84; // Without the final '\n' !

static const int SL_NB_LINES = 5000;
static const int SL_LINE_PER_PAGE = 70;
static const char* sl_format="LOGDATA is a part of glogg, we are going to test it thoroughly, this is line %06d\n";
static const int SL_LINE_LENGTH = 83; // Without the final '\n' !

void TestLogData::initTestCase()
{
    QVERIFY( generateDataFiles() );
}

void TestLogData::simpleLoad()
{
    LogData logData;
    QSignalSpy progressSpy( &logData, SIGNAL( loadingProgressed( int ) ) );

    // Register for notification file is loaded
    connect( &logData, SIGNAL( loadingFinished() ),
            this, SLOT( loadingFinished() ) );

    QBENCHMARK {
        QVERIFY( logData.attachFile( TMPDIR "/verybiglog.txt" ) );
        // Wait for the loading to be done
        {
            QApplication::exec();
        }
    }
    QCOMPARE( (qint64) progressSpy.count(), logData.getFileSize() / (5LL*1024*1024) + 1 );
    QCOMPARE( logData.getNbLine(), VBL_NB_LINES );
    QCOMPARE( logData.getMaxLength(), VBL_LINE_LENGTH );
    QCOMPARE( logData.getFileSize(), VBL_NB_LINES * (VBL_LINE_LENGTH+1LL) );

    // Disconnect all signals
    disconnect( &logData, 0 );
}

void TestLogData::multipleLoad()
{
    LogData logData;
    QSignalSpy finishedSpy( &logData, SIGNAL( loadingFinished() ) );

    // Register for notification file is loaded
    connect( &logData, SIGNAL( loadingFinished() ),
            this, SLOT( loadingFinished() ) );

    // Start loading the VBL
    QVERIFY( logData.attachFile( TMPDIR "/verybiglog.txt" ) );

    // Immediately interrupt the loading
    logData.interruptLoading();

    // and wait for the signal
    QApplication::exec();

    // Check we have an empty file
    QCOMPARE( finishedSpy.count(), 1 );
    QCOMPARE( logData.getNbLine(), 0 );
    QCOMPARE( logData.getMaxLength(), 0 );
    QCOMPARE( logData.getFileSize(), 0LL );

    // Restart the VBL
    QVERIFY( logData.attachFile( TMPDIR "/verybiglog.txt" ) );

    // Ensure the counting has started
    {
        QMutex mutex;
        QWaitCondition sleep;
        // sleep.wait( &mutex, 10 );
    }

    // Load the SL (should block until VBL is fully indexed)
    QVERIFY( logData.attachFile( TMPDIR "/smalllog.txt" ) );

    // and wait for the 2 signals (one for each file)
    QApplication::exec();
    QApplication::exec();

    // Check we have the small log loaded
    QCOMPARE( finishedSpy.count(), 3 );
    QCOMPARE( logData.getNbLine(), SL_NB_LINES );
    QCOMPARE( logData.getMaxLength(), SL_LINE_LENGTH );
    QCOMPARE( logData.getFileSize(), SL_NB_LINES * (SL_LINE_LENGTH+1LL) );

    // Restart the VBL again
    QVERIFY( logData.attachFile( TMPDIR "/verybiglog.txt" ) );

    // Immediately interrupt the loading
    logData.interruptLoading();

    // and wait for the signal
    QApplication::exec();

    // Check the small log has been restored
    QCOMPARE( finishedSpy.count(), 4 );
    QCOMPARE( logData.getNbLine(), SL_NB_LINES );
    QCOMPARE( logData.getMaxLength(), SL_LINE_LENGTH );
    QCOMPARE( logData.getFileSize(), SL_NB_LINES * (SL_LINE_LENGTH+1LL) );

    // Disconnect all signals
    disconnect( &logData, 0 );
}

void TestLogData::changingFile()
{
    char newLine[90];
    LogData logData;

    QSignalSpy finishedSpy( &logData, SIGNAL( loadingFinished() ) );
    QSignalSpy progressSpy( &logData, SIGNAL( loadingProgressed( int ) ) );
    QSignalSpy changedSpy( &logData,
            SIGNAL( fileChanged( LogData::MonitoredFileStatus ) ) );

    // Register for notification file is loaded
    connect( &logData, SIGNAL( loadingFinished() ),
            this, SLOT( loadingFinished() ) );

    // Generate a small file
    QFile file( TMPDIR "/changingfile.txt" );
    if ( file.open( QIODevice::WriteOnly ) ) {
        for (int i = 0; i < 200; i++) {
            snprintf(newLine, 89, sl_format, i);
            file.write( newLine, qstrlen(newLine) );
        }
    }
    file.close();

    // Start loading it
    QVERIFY( logData.attachFile( TMPDIR "/changingfile.txt" ) );

    // and wait for the signal
    QApplication::exec();

    // Check we have the small file
    QCOMPARE( finishedSpy.count(), 1 );
    QCOMPARE( logData.getNbLine(), 200 );
    QCOMPARE( logData.getMaxLength(), SL_LINE_LENGTH );
    QCOMPARE( logData.getFileSize(), 200 * (SL_LINE_LENGTH+1LL) );

    // Add some data to it
    if ( file.open( QIODevice::Append ) ) {
        for (int i = 0; i < 200; i++) {
            snprintf(newLine, 89, sl_format, i);
            file.write( newLine, qstrlen(newLine) );
        }
    }
    file.close();

    // and wait for the signals
    QApplication::exec();

    // Check we have a bigger file
    QCOMPARE( changedSpy.count(), 1 );
    QCOMPARE( finishedSpy.count(), 2 );
    QCOMPARE( logData.getNbLine(), 400 );
    QCOMPARE( logData.getMaxLength(), SL_LINE_LENGTH );
    QCOMPARE( logData.getFileSize(), 400 * (SL_LINE_LENGTH+1LL) );

    // Truncate the file
    QVERIFY( file.open( QIODevice::WriteOnly ) );
    file.close();

    // and wait for the signals
    QApplication::exec();

    // Check we have an empty file
    QCOMPARE( changedSpy.count(), 2 );
    QCOMPARE( finishedSpy.count(), 3 );
    QCOMPARE( logData.getNbLine(), 0 );
    QCOMPARE( logData.getMaxLength(), 0 );
    QCOMPARE( logData.getFileSize(), 0LL );
}

void TestLogData::sequentialRead()
{
    LogData logData;

    // Register for notification file is loaded
    connect( &logData, SIGNAL( loadingFinished() ),
            this, SLOT( loadingFinished() ) );

    QVERIFY( logData.attachFile( TMPDIR "/verybiglog.txt" ) );

    // Wait for the loading to be done
    {
        QApplication::exec();
    }

    // Read all lines sequentially
    QString s;
    QBENCHMARK {
        for (int i = 0; i < VBL_NB_LINES; i++) {
            s = logData.getLineString(i);
        }
    }
    QCOMPARE( s.length(), VBL_LINE_LENGTH );
}

void TestLogData::randomPageRead()
{
    LogData logData;

    // Register for notification file is loaded
    connect( &logData, SIGNAL( loadingFinished() ),
            this, SLOT( loadingFinished() ) );

    logData.attachFile( TMPDIR "/verybiglog.txt" );
    // Wait for the loading to be done
    {
        QApplication::exec();
    }

    QWARN("Starting random page read test");

    // Read page by page from the beginning and the end, using the QStringList
    // function
    QStringList list;
    QBENCHMARK {
        for (int page = 0; page < (VBL_NB_LINES/VBL_LINE_PER_PAGE)-1; page++)
        {
            list = logData.getLines( page*VBL_LINE_PER_PAGE, VBL_LINE_PER_PAGE );
            QCOMPARE(list.count(), VBL_LINE_PER_PAGE);
            int page_from_end = (VBL_NB_LINES/VBL_LINE_PER_PAGE) - page - 1;
            list = logData.getLines( page_from_end*VBL_LINE_PER_PAGE, VBL_LINE_PER_PAGE );
            QCOMPARE(list.count(), VBL_LINE_PER_PAGE);
        }
    }
}

//
// Private functions
//
void TestLogData::loadingFinished()
{
    QApplication::quit();
}

bool TestLogData::generateDataFiles()
{
    char newLine[90];

    QFile file( TMPDIR "/verybiglog.txt" );
    if ( file.open( QIODevice::WriteOnly ) ) {
        for (int i = 0; i < VBL_NB_LINES; i++) {
            snprintf(newLine, 89, vbl_format, i);
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
