#include <QTest>
#include <QSignalSpy>
#include <QTemporaryFile>

#include "log.h"
#include "test_utils.h"

#include "data/logdata.h"
#include "data/logfiltereddata.h"

#include <catch.hpp>

static const qint64 SL_NB_LINES = 500LL;
static const char* sl_format="LOGDATA is a part of glogg, we are going to test it thoroughly, this is line %06d\n";

namespace {

bool generateDataFiles(QTemporaryFile& file) {
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

void runSearch( LogFilteredData* filtered_data,  const QString& regexp,
                SafeQSignalSpy& searchProgressSpy ) {

    filtered_data->runSearch( QRegularExpression( regexp ) );

    int progress = 0;
    do
    {
        REQUIRE( searchProgressSpy.wait() );
        QList<QVariant> progressArgs = searchProgressSpy.last();
        progress = progressArgs.at(1).toInt();
    } while ( progress < 100 );
}

}

SCENARIO( "filtered log data", "[logdata]") {

    GIVEN( "loaded log data" ) {
        QTemporaryFile file;
        REQUIRE( generateDataFiles( file ) );
        LogData log_data;
        SafeQSignalSpy loadEndSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

        log_data.attachFile( file.fileName() );
        REQUIRE( loadEndSpy.safeWait( 10000 ) );

        auto filtered_data = std::unique_ptr<LogFilteredData>( log_data.getNewFilteredData() );

        WHEN( "Adding mark outside file" ) {
            filtered_data->addMark( LineNumber( SL_NB_LINES + 25 ) );

            THEN( "No marked lines stored" ) {
                for ( LineNumber i = 0_lnum; i < LineNumber( SL_NB_LINES ); ++i )
                    REQUIRE_FALSE( filtered_data->isLineMarked( i ) );
            }
        }

        WHEN( "Adding marks in log file" ) {
            filtered_data->addMark( 10_lnum );
            filtered_data->addMark( 25_lnum );

            THEN( "Marks are stored" ) {
                REQUIRE( filtered_data->isLineMarked( 10_lnum ) );
                REQUIRE( filtered_data->isLineMarked( 25_lnum ) );
                REQUIRE( filtered_data->getNbMarks() == 2_lcount );
            }
        }

        WHEN( "Searched for regex" ) {
            auto filtered_lines = filtered_data->getNbLine();
            REQUIRE( filtered_lines.get() == 0);

            SafeQSignalSpy searchProgressSpy {
                        filtered_data.get(),
                        &LogFilteredData::searchProgressed
            };

            runSearch( filtered_data.get(), "this is line [0-9]{5}9", searchProgressSpy );

            THEN( "Matched lines are in data" ) {
                QList<QVariant> progressArgs = searchProgressSpy.last();
                REQUIRE( qvariant_cast<LinesCount>( progressArgs.at(0) ) == 50_lcount );

                const auto matches_count = filtered_data->getNbMatches();
                REQUIRE( matches_count == 50_lcount );

                const auto lines = filtered_data->getExpandedLines(0_lnum,
                                                                   matches_count );
                for ( const auto& l: lines) {
                    REQUIRE( l.endsWith( '9' ) );
                }
            }
        }

        WHEN( "Add marks at matched line" ) {
            SafeQSignalSpy searchProgressSpy {
                        filtered_data.get(),
                        &LogFilteredData::searchProgressed
            };

            runSearch( filtered_data.get(), "this is line [0-9]{5}9", searchProgressSpy );
            filtered_data->addMark( 9_lnum );

            THEN( "Has same number of lines" ) {
                REQUIRE( filtered_data->getNbLine() == 50_lcount );
            }
        }

        WHEN( "Add marks at not matched line" ) {
            filtered_data->addMark( 9_lnum );


            SafeQSignalSpy searchProgressSpy {
                        filtered_data.get(),
                        &LogFilteredData::searchProgressed
            };

            runSearch( filtered_data.get(), "this is line [0-9]{5}5", searchProgressSpy );

            THEN( "Has one more line" ) {
                REQUIRE( filtered_data->getNbLine() == 51_lcount );
            }
        }

        WHEN( "Only marks are visible" ) {
            SafeQSignalSpy searchProgressSpy {
                        filtered_data.get(),
                        &LogFilteredData::searchProgressed
            };

            runSearch( filtered_data.get(), "this is line [0-9]{5}5", searchProgressSpy );
            filtered_data->addMark( 9_lnum );
            filtered_data->addMark( 5_lnum );

            filtered_data->setVisibility( LogFilteredData::MarksOnly );

            THEN( "Has only marked lines count" ) {
                REQUIRE( filtered_data->getNbLine() == 2_lcount );
            }
        }

        WHEN( "Only mathes are visible" ) {
            SafeQSignalSpy searchProgressSpy {
                        filtered_data.get(),
                        &LogFilteredData::searchProgressed
            };

            runSearch( filtered_data.get(), "this is line [0-9]{5}5", searchProgressSpy );
            filtered_data->addMark( 9_lnum );
            filtered_data->addMark( 5_lnum );

            filtered_data->setVisibility( LogFilteredData::MatchesOnly );

            THEN( "Has only matches lines count" ) {
                REQUIRE( filtered_data->getNbLine() == 50_lcount );
            }
        }

        WHEN( "Ask for matching line number" ) {
            SafeQSignalSpy searchProgressSpy {
                        filtered_data.get(),
                        &LogFilteredData::searchProgressed
            };

            runSearch( filtered_data.get(), "this is line [0-9]{5}9", searchProgressSpy );
            filtered_data->addMark( 1_lnum );

            WHEN( "For marked line" ) {
                auto original_line = filtered_data->getMatchingLineNumber( 0_lnum );
                THEN( "Original line is on mark" ) {
                    REQUIRE( original_line == 1_lnum );
                }
            }

            WHEN( "For matched line" ) {
                auto original_line = filtered_data->getMatchingLineNumber( 1_lnum );
                THEN( "Original line is on match" ) {
                    REQUIRE( original_line == 9_lnum );
                }
            }

            WHEN( "For last line" ) {
                auto max_filtered_line = LineNumber( filtered_data->getNbLine().get() - 1 );
                auto original_line = filtered_data->getMatchingLineNumber( max_filtered_line );
                THEN( "Original line is last" ) {
                    REQUIRE( original_line == 499_lnum );
                }
            }

            WHEN( "For invalid line" ) {
                auto max_filtered_line = LineNumber( filtered_data->getNbLine().get() - 1 );
                auto original_line = filtered_data->getMatchingLineNumber( max_filtered_line + 1_lcount );
                THEN( "Max line number is returned" ) {
                    REQUIRE( original_line == maxValue<LineNumber>() );
                }
            }
        }

        WHEN( "Ask for filtered line index" ) {
            SafeQSignalSpy searchProgressSpy {
                        filtered_data.get(),
                        &LogFilteredData::searchProgressed
            };

            runSearch( filtered_data.get(), "this is line [0-9]{5}9", searchProgressSpy );
            filtered_data->addMark( 1_lnum );

            WHEN( "For marked line" ) {
                auto filtered_line = filtered_data->getLineIndexNumber( 1_lnum );
                THEN( "Marked line returned" ) {
                    REQUIRE( filtered_line == 0_lnum );
                }
            }

            WHEN( "For matched line" ) {
                auto filtered_line = filtered_data->getLineIndexNumber( 9_lnum );
                THEN( "Matched line returned" ) {
                    REQUIRE( filtered_line == 1_lnum );
                }
            }

            WHEN( "For last line" ) {
                auto max_filtered_line = LineNumber( filtered_data->getNbLine().get() - 1 );
                auto filtered_line = filtered_data->getLineIndexNumber( 499_lnum );
                THEN( "Last matched line returned" ) {
                    REQUIRE( filtered_line == max_filtered_line );
                }
            }

            WHEN( "For invalid line" ) {
                auto filtered_line = filtered_data->getMatchingLineNumber( 500_lnum );
                THEN( "Max line number is returned" ) {
                    REQUIRE( filtered_line == maxValue<LineNumber>() );
                }
            }
        }
    }
}

#if 0
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

TEST_F( FilteredLogDataFixture, getMatchingLineNumber ) {
    SafeQSignalSpy searchProgressSpy {
                filtered_data.get(),
                &LogFilteredData::searchProgressed
    };

    runSearch( "this is line [0-9]{5}9", searchProgressSpy );

    filtered_data->addMark( 1_lnum );

    auto original_line = filtered_data->getMatchingLineNumber( 0_lnum );

    ASSERT_EQ( original_line, 1_lnum );

    auto max_filtered_line = LineNumber( filtered_data->getNbLine().get() - 1 );
    original_line = filtered_data->getMatchingLineNumber( max_filtered_line );
    ASSERT_EQ( original_line, 499_lnum );

    original_line = filtered_data->getMatchingLineNumber( max_filtered_line + 1_lcount );
    ASSERT_EQ( original_line, maxValue<LineNumber>() );
}

TEST_F( FilteredLogDataFixture, getLineIndexNumber ) {
    SafeQSignalSpy searchProgressSpy {
                filtered_data.get(),
                &LogFilteredData::searchProgressed
    };

    runSearch( "this is line [0-9]{5}9", searchProgressSpy );

    filtered_data->addMark( 1_lnum );

    auto filtered_line = filtered_data->getLineIndexNumber( 1_lnum );

    ASSERT_EQ( filtered_line, 0_lnum );

    auto max_filtered_line = LineNumber( filtered_data->getNbLine().get() - 1 );
    filtered_line = filtered_data->getLineIndexNumber( 499_lnum );
    ASSERT_EQ( filtered_line, max_filtered_line );

    filtered_line = filtered_data->getMatchingLineNumber( 500_lnum );
    ASSERT_EQ( filtered_line, maxValue<LineNumber>() );
}

#endif


