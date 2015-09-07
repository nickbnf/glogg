#include <QTest>
#include <QSignalSpy>

#include "log.h"
#include "test_utils.h"

#include "data/logdata.h"
#include "data/logfiltereddata.h"

#include "gmock/gmock.h"

#define TMPDIR "/tmp"

static const qint64 SL_NB_LINES = 5000LL;
static const int SL_LINE_PER_PAGE = 70;
static const char* sl_format="LOGDATA is a part of glogg, we are going to test it thoroughly, this is line %06d\n";
static const int SL_LINE_LENGTH = 83; // Without the final '\n' !

class MarksBehaviour : public testing::Test {
  public:
    LogData log_data;
    SafeQSignalSpy endSpy;
    LogFilteredData* filtered_data = nullptr;

    MarksBehaviour() : endSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) ) {
        generateDataFiles();

        log_data.attachFile( TMPDIR "/smalllog.txt" );
        endSpy.safeWait( 10000 );

        filtered_data = log_data.getNewFilteredData();
    }


    bool generateDataFiles() {
        char newLine[90];

        QFile file( TMPDIR "/smalllog.txt" );
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
};

TEST_F( MarksBehaviour, marksIgnoresOutOfLimitLines ) {
    filtered_data->addMark( -10 );
    filtered_data->addMark( SL_NB_LINES + 25 );

    // Check we still have no mark
    for ( int i = 0; i < SL_NB_LINES; i++ )
        ASSERT_FALSE( filtered_data->isLineMarked( i ) );
}

TEST_F( MarksBehaviour, marksAreStored ) {
    filtered_data->addMark( 10 );
    filtered_data->addMark( 25 );

    // Check the marks have been put
    ASSERT_TRUE( filtered_data->isLineMarked( 10 ) );
    ASSERT_TRUE( filtered_data->isLineMarked( 25 ) );
}
