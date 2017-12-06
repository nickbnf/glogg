#include <QTest>
#include <QSignalSpy>
#include <QTemporaryFile>

#include "log.h"
#include "test_utils.h"

#include "data/logdata.h"
#include "data/logfiltereddata.h"

#include "gmock/gmock.h"

static const qint64 SL_NB_LINES = 500LL;
static const char* sl_format="LOGDATA is a part of glogg, we are going to test it thoroughly, this is line %06d\n";

class FilteredLogDataFixture : public testing::Test {
  public:
    FilteredLogDataFixture()
        : loadEndSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) )
    {}

    void SetUp() override
    {
        generateDataFiles();

        log_data.attachFile( file.fileName() );
        ASSERT_TRUE( loadEndSpy.safeWait( 10000 ) );

        filtered_data.reset( log_data.getNewFilteredData() );
    }

  protected:
    LogData log_data;
    std::unique_ptr<LogFilteredData> filtered_data;

    void runSearch( const QString& regexp,
                    SafeQSignalSpy& searchProgressSpy ) {
        filtered_data->runSearch( QRegularExpression( regexp ) );

        int progress = 0;
        do
        {
            ASSERT_TRUE( searchProgressSpy.wait() );
            QList<QVariant> progressArgs = searchProgressSpy.last();
            progress = progressArgs.at(1).toInt();
        } while ( progress < 100 );
    }

  private:
    SafeQSignalSpy loadEndSpy;

    QTemporaryFile file;

    bool generateDataFiles() {
        char newLine[90];

        if ( file.open() ) {
            for (int i = 0; i < SL_NB_LINES; i++) {
                snprintf(newLine, 89, sl_format, i);
                file.write( newLine, qstrlen(newLine) );
            }
            file.flush();
        }

        return true;
    }
};

TEST_F( FilteredLogDataFixture, marksIgnoresOutOfLimitLines ) {
    filtered_data->addMark( LineNumber( SL_NB_LINES + 25 ) );

    // Check we still have no mark
    for ( LineNumber i = 0_lnum; i < LineNumber( SL_NB_LINES ); ++i )
        ASSERT_FALSE( filtered_data->isLineMarked( i ) );
}

TEST_F( FilteredLogDataFixture, marksAreStored ) {
    filtered_data->addMark( 10_lnum );
    filtered_data->addMark( 25_lnum );

    // Check the marks have been put
    ASSERT_TRUE( filtered_data->isLineMarked( 10_lnum ) );
    ASSERT_TRUE( filtered_data->isLineMarked( 25_lnum ) );

    ASSERT_EQ( filtered_data->getNbMarks(), 2_lcount );
}

TEST_F( FilteredLogDataFixture, searchReturnsMatchedLines ) {
    auto filtered_lines = filtered_data->getNbLine();
    ASSERT_EQ( filtered_lines.get(), 0);

    SafeQSignalSpy searchProgressSpy {
                filtered_data.get(),
                &LogFilteredData::searchProgressed
    };

    runSearch( "this is line [0-9]{5}9", searchProgressSpy );

    QList<QVariant> progressArgs = searchProgressSpy.last();
    ASSERT_EQ( qvariant_cast<LinesCount>( progressArgs.at(0) ), 50_lcount );

    const auto matches_count = filtered_data->getNbMatches();
    ASSERT_EQ( matches_count.get(), 50 );

    const auto lines = filtered_data->getExpandedLines(0_lnum,
                                                       matches_count );
    for ( const auto& l: lines) {
        ASSERT_TRUE( l.endsWith( '9' ) );
    }
}

TEST_F( FilteredLogDataFixture, matchesAndMarksAddUpCount ) {
    SafeQSignalSpy searchProgressSpy {
                filtered_data.get(),
                &LogFilteredData::searchProgressed
    };

    runSearch( "this is line [0-9]{5}9", searchProgressSpy );

    filtered_data->addMark( 9_lnum );

    auto filtered_count = filtered_data->getNbLine();
    ASSERT_EQ( filtered_count.get(), 50 );

    runSearch( "this is line [0-9]{5}5", searchProgressSpy );

    filtered_count = filtered_data->getNbLine();
    ASSERT_EQ( filtered_count.get(), 51 );
}

TEST_F( FilteredLogDataFixture, visibilityChangesCount ) {
    SafeQSignalSpy searchProgressSpy {
                filtered_data.get(),
                &LogFilteredData::searchProgressed
    };

    runSearch( "this is line [0-9]{5}9", searchProgressSpy );

    filtered_data->addMark( 0_lnum );

    auto filtered_count = filtered_data->getNbLine();
    ASSERT_EQ( filtered_count.get(), 51 );

    filtered_data->setVisibility( LogFilteredData::MatchesOnly );
    filtered_count = filtered_data->getNbLine();
    ASSERT_EQ( filtered_count.get(), 50 );

    filtered_data->setVisibility( LogFilteredData::MarksOnly );
    filtered_count = filtered_data->getNbLine();
    ASSERT_EQ( filtered_count.get(), 1 );
}






