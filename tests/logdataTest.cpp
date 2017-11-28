#include <iostream>

#include <QTest>
#include <QSignalSpy>
#include <QTemporaryFile>

#include "log.h"
#include "test_utils.h"

#include "data/logdata.h"

#include "gmock/gmock.h"

static const qint64 SL_NB_LINES = 500LL;
static const qint64 VBL_NB_LINES = 5000LL;

static const int SL_LINE_PER_PAGE = 70;
static const char* sl_format="LOGDATA is a part of glogg, we are going to test it thoroughly, this is line %06d\n";
static const int SL_LINE_LENGTH = 83; // Without the final '\n' !

static const char* partial_line_begin = "123... beginning of line.";
static const char* partial_line_end = " end of line 123.\n";

namespace {

void writeDataToFile( QFile& file, int numberOfLines = 200, bool flush = true ) {
    char newLine[90];

    for (int i = 0; i < numberOfLines; i++) {
        snprintf(newLine, 89, sl_format, i);
        file.write( newLine, qstrlen(newLine) );
    }

    if ( flush ) {
        file.flush();
    }
}

}


class LogDataChanging : public testing::Test {
};

TEST_F( LogDataChanging, changingFile ) {
    LogData log_data;

    SafeQSignalSpy finishedSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );
    SafeQSignalSpy progressSpy( &log_data, SIGNAL( loadingProgressed( int ) ) );
    SafeQSignalSpy changedSpy( &log_data,
            SIGNAL( fileChanged( LogData::MonitoredFileStatus ) ) );

    // Generate a small file
    QTemporaryFile file;
    if ( file.open() ) {
        writeDataToFile( file );
    }

    // Start loading it
    log_data.attachFile( file.fileName() );

    // and wait for the signal
    ASSERT_TRUE( finishedSpy.safeWait() );

    // Check we have the small file
    ASSERT_GE( finishedSpy.count(), 1 );
    ASSERT_THAT( log_data.getNbLine(), 200LL );
    ASSERT_THAT( log_data.getMaxLength(), SL_LINE_LENGTH );
    ASSERT_THAT( log_data.getFileSize(), 200 * (SL_LINE_LENGTH+1LL) );

    // Add some data to it
    if ( file.open() ) {
        writeDataToFile( file );
        // To test the edge case when the final line is not complete
        file.write( partial_line_begin, qstrlen( partial_line_begin ) );
        file.flush();
    }

    // and wait for the signals
    ASSERT_TRUE( finishedSpy.wait( 10000 ) );

    // Check we have a bigger file
    ASSERT_THAT( changedSpy.count(), 1 );
    ASSERT_THAT( finishedSpy.count(), 2 );
    ASSERT_THAT( log_data.getNbLine(), 401LL );
    ASSERT_THAT( log_data.getMaxLength(), SL_LINE_LENGTH );
    ASSERT_THAT( log_data.getFileSize(), (qint64) (400 * (SL_LINE_LENGTH+1LL)
            + strlen( partial_line_begin ) ) );

    {
        SafeQSignalSpy finishedSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

        // Add a couple more lines, including the end of the unfinished one.
        if ( file.open() ) {
            file.write( partial_line_end, qstrlen( partial_line_end ) );
            writeDataToFile( file, 20 );
        }
        file.close();

        // and wait for the signals
        ASSERT_TRUE( finishedSpy.wait( 10000 ) );

        // Check we have a bigger file
        ASSERT_GE( changedSpy.count(), 2 );
        ASSERT_THAT( finishedSpy.count(), 1 );
        ASSERT_THAT( log_data.getNbLine(), 421LL );
        ASSERT_THAT( log_data.getMaxLength(), SL_LINE_LENGTH );
        ASSERT_THAT( log_data.getFileSize(), (qint64) ( 420 * (SL_LINE_LENGTH+1LL)
                + strlen( partial_line_begin ) + strlen( partial_line_end ) ) );
    }

    {
        SafeQSignalSpy finishedSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

        // Truncate the file
        QVERIFY( file.resize( 0 ) );

        // and wait for the signals
        ASSERT_TRUE( finishedSpy.safeWait() );

        // Check we have an empty file
        ASSERT_GE( changedSpy.count(), 3 );
        ASSERT_THAT( finishedSpy.count(), 1 );
        ASSERT_THAT( log_data.getNbLine(), 0LL );
        ASSERT_THAT( log_data.getMaxLength(), 0 );
        ASSERT_THAT( log_data.getFileSize(), 0LL );
    }
}

class LogDataBehaviour : public testing::Test {
  public:
    LogDataBehaviour() {
        if ( smallFile_.open() ) {
           writeDataToFile( smallFile_, SL_NB_LINES );
        }

        if ( bigFile_.open() ) {
            writeDataToFile( bigFile_, VBL_NB_LINES );
        }
    }

protected:
    QTemporaryFile smallFile_;
    QTemporaryFile bigFile_;
};

TEST_F( LogDataBehaviour, interruptLoadYieldsAnEmptyFile ) {
    LogData log_data;
    SafeQSignalSpy endSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

    // Start loading the VBL
    log_data.attachFile( bigFile_.fileName() );

    // Immediately interrupt the loading
    log_data.interruptLoading();

    ASSERT_TRUE( endSpy.safeWait( 10000 ) );

    // Check we have an empty file
    ASSERT_THAT( endSpy.count(), 1 );
    QList<QVariant> arguments = endSpy.takeFirst();
    ASSERT_THAT( arguments.at(0).toInt(),
            static_cast<int>( LoadingStatus::Interrupted ) );

    ASSERT_THAT( log_data.getNbLine(), 0LL );
    ASSERT_THAT( log_data.getMaxLength(), 0 );
    ASSERT_THAT( log_data.getFileSize(), 0LL );
}

TEST_F( LogDataBehaviour, cannotBeReAttached ) {
    LogData log_data;
    SafeQSignalSpy endSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

    log_data.attachFile( smallFile_.fileName() );
    endSpy.safeWait( 10000 );

    ASSERT_THROW( log_data.attachFile( bigFile_.fileName() ), CantReattachErr );
}
