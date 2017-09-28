#include <iostream>

#include <QTest>
#include <QSignalSpy>

#include "log.h"
#include "test_utils.h"

#include "data/logdata.h"

#include "gmock/gmock.h"

#define TMPDIR "/tmp"

static const qint64 SL_NB_LINES = 5000LL;
static const int SL_LINE_PER_PAGE = 70;
static const char* sl_format="LOGDATA is a part of glogg, we are going to test it thoroughly, this is line %06d\n";
static const int SL_LINE_LENGTH = 83; // Without the final '\n' !

static const char* partial_line_begin = "123... beginning of line.";
static const char* partial_line_end = " end of line 123.\n";


class LogDataChanging : public testing::Test {
  public:
};

TEST_F( LogDataChanging, changingFile ) {
    char newLine[90];
    LogData log_data;

    SafeQSignalSpy finishedSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );
    SafeQSignalSpy progressSpy( &log_data, SIGNAL( loadingProgressed( int ) ) );
    SafeQSignalSpy changedSpy( &log_data,
            SIGNAL( fileChanged( LogData::MonitoredFileStatus ) ) );

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
    log_data.attachFile( TMPDIR "/changingfile.txt" );

    // and wait for the signal
    ASSERT_TRUE( finishedSpy.safeWait() );

    // Check we have the small file
    ASSERT_THAT( finishedSpy.count(), 1 );
    ASSERT_THAT( log_data.getNbLine(), 200LL );
    ASSERT_THAT( log_data.getMaxLength(), SL_LINE_LENGTH );
    ASSERT_THAT( log_data.getFileSize(), 200 * (SL_LINE_LENGTH+1LL) );

    // Add some data to it
    if ( file.open( QIODevice::Append ) ) {
        for (int i = 0; i < 200; i++) {
            snprintf(newLine, 89, sl_format, i);
            file.write( newLine, qstrlen(newLine) );
        }
        // To test the edge case when the final line is not complete
        file.write( partial_line_begin, qstrlen( partial_line_begin ) );
    }
    file.close();

    // and wait for the signals
    ASSERT_TRUE( finishedSpy.wait( 1000 ) );

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
        if ( file.open( QIODevice::Append ) ) {
            file.write( partial_line_end, qstrlen( partial_line_end ) );
            for (int i = 0; i < 20; i++) {
                snprintf(newLine, 89, sl_format, i);
                file.write( newLine, qstrlen(newLine) );
            }
        }
        file.close();

        // and wait for the signals
        ASSERT_TRUE( finishedSpy.wait( 1000 ) );

        // Check we have a bigger file
        ASSERT_THAT( changedSpy.count(), 2 );
        ASSERT_THAT( finishedSpy.count(), 1 );
        ASSERT_THAT( log_data.getNbLine(), 421LL );
        ASSERT_THAT( log_data.getMaxLength(), SL_LINE_LENGTH );
        ASSERT_THAT( log_data.getFileSize(), (qint64) ( 420 * (SL_LINE_LENGTH+1LL)
                + strlen( partial_line_begin ) + strlen( partial_line_end ) ) );
    }

    {
        SafeQSignalSpy finishedSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

        // Truncate the file
        QVERIFY( file.open( QIODevice::WriteOnly ) );
        file.close();

        // and wait for the signals
        ASSERT_TRUE( finishedSpy.safeWait() );

        // Check we have an empty file
        ASSERT_THAT( changedSpy.count(), 3 );
        ASSERT_THAT( finishedSpy.count(), 1 );
        ASSERT_THAT( log_data.getNbLine(), 0LL );
        ASSERT_THAT( log_data.getMaxLength(), 0 );
        ASSERT_THAT( log_data.getFileSize(), 0LL );
    }
}

class LogDataBehaviour : public testing::Test {
  public:
    LogDataBehaviour() {
        generateDataFiles();
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

TEST_F( LogDataBehaviour, interruptLoadYieldsAnEmptyFile ) {
    LogData log_data;
    SafeQSignalSpy endSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

    // Start loading the VBL
    log_data.attachFile( TMPDIR "/verybiglog.txt" );

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

    log_data.attachFile( TMPDIR "/smalllog.txt" );
    endSpy.safeWait( 10000 );

    ASSERT_THROW( log_data.attachFile( TMPDIR "/verybiglog.txt" ), CantReattachErr );
}

TEST_F( LogDataBehaviour, readFunctions ) {
    LogData log_data;
    SafeQSignalSpy endSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

    log_data.attachFile( TMPDIR "/smalllog.txt" );
    endSpy.safeWait( 10000 );

    QString ref = QString::fromUtf8( "LOGDATA is a part of glogg, we are going to test it thoroughly, this is line 000012" );

    ASSERT_THAT( QString::compare( log_data.getLineString( 12 ), ref ), 0 );
    ASSERT_THAT( QString::compare( log_data.getExpandedLineString( 12 ), ref ), 0 );
    ASSERT_THAT( QString::compare( log_data.getLines( 11, 3 ).at( 1 ), ref ), 0 );
    ASSERT_THAT( QString::compare( log_data.getExpandedLines( 12, 2 ).at( 0 ), ref ), 0 );
}

class LogDataMultiByte : public testing::Test {
  public:
    LogDataMultiByte() {
        generateDataFiles();
    }

    bool generateDataFiles() {
        const QString text = QString::fromUtf8( u8"DOM JUAN\n\
ou LE FESTIN DE PIERRE\n\
COMÉDIE\n\
1665\n\
Molière\n\
\n\
\n\
Représentée pour la première fois le 15 février 1665 sur le Théâtre de la salle du Palais-Royal par la Troupe de Monsieur, frère unique du Roi.\n\
PERSONNAGES\n\
DOM JUAN, fils de Dom Louis.\n\
SGANARELLE, valet de Dom Juan.\n\
ELVIRE, femme de Dom Juan.\n\
GUSMAN, écuyer d'Elvire.\n\
DOM CARLOS, frère d'Elvire.\n\
DOM ALFONSE, frère d'Elvire\n\
DOM LOUIS, père de Dom Juan.\n" );

        QFile file_utf16le( TMPDIR "/utf16le.txt" );
        QTextCodec *codec_utf16le = QTextCodec::codecForName( "UTF16LE" );
        if ( file_utf16le.open( QIODevice::WriteOnly ) ) {
            file_utf16le.write( codec_utf16le->fromUnicode( text ) );
        }
        else {
            return false;
        }
        file_utf16le.close();

        QFile file_utf16be( TMPDIR "/utf16be.txt" );
        QTextCodec *codec_utf16be = QTextCodec::codecForName( "UTF16BE" );
        if ( file_utf16be.open( QIODevice::WriteOnly ) ) {
            file_utf16be.write( codec_utf16be->fromUnicode( text ) );
        }
        else {
            return false;
        }
        file_utf16be.close();

        return true;
    }
};

TEST_F( LogDataMultiByte, readUtf16LE ) {
    LogData log_data;
    SafeQSignalSpy endSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

    log_data.attachFile( TMPDIR "/utf16le.txt" );
    endSpy.safeWait( 10000 );

    log_data.setDisplayEncoding( Encoding::ENCODING_UTF16LE );

    ASSERT_THAT( log_data.getDetectedEncoding(), EncodingSpeculator::Encoding::UTF16LE );
    ASSERT_THAT( QString::compare( log_data.getLineString( 3 ), QStringLiteral( "1665" ) ), 0 );
    ASSERT_THAT( QString::compare( log_data.getExpandedLineString( 4 ), QStringLiteral( "Molière" ) ), 0 );
    ASSERT_THAT( QString::compare( log_data.getLines( 11, 3 ).at( 0 ), QStringLiteral( "ELVIRE, femme de Dom Juan." ) ), 0 );
    ASSERT_THAT( QString::compare( log_data.getLines( 11, 3 ).at( 1 ), QStringLiteral( "GUSMAN, écuyer d'Elvire." ) ), 0 );
    ASSERT_THAT( QString::compare( log_data.getLines( 11, 3 ).at( 2 ), QStringLiteral( "DOM CARLOS, frère d'Elvire." ) ), 0 );
    ASSERT_THAT( QString::compare( log_data.getExpandedLines( 0, 3 ).at( 2 ), QStringLiteral( "COMÉDIE" ) ), 0 );
}

TEST_F( LogDataMultiByte, readUtf16BE ) {
    LogData log_data;
    SafeQSignalSpy endSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

    log_data.attachFile( TMPDIR "/utf16be.txt" );
    endSpy.safeWait( 10000 );

    log_data.setDisplayEncoding( Encoding::ENCODING_UTF16BE );

    ASSERT_THAT( log_data.getDetectedEncoding(), EncodingSpeculator::Encoding::UTF16BE );
    ASSERT_THAT( QString::compare( log_data.getLineString( 3 ), QStringLiteral( "1665" ) ), 0 );
    ASSERT_THAT( QString::compare( log_data.getExpandedLineString( 4 ), QStringLiteral( "Molière" ) ), 0 );
    ASSERT_THAT( QString::compare( log_data.getLines( 11, 3 ).at( 0 ), QStringLiteral( "ELVIRE, femme de Dom Juan." ) ), 0 );
    ASSERT_THAT( QString::compare( log_data.getLines( 11, 3 ).at( 1 ), QStringLiteral( "GUSMAN, écuyer d'Elvire." ) ), 0 );
    ASSERT_THAT( QString::compare( log_data.getLines( 11, 3 ).at( 2 ), QStringLiteral( "DOM CARLOS, frère d'Elvire." ) ), 0 );
    ASSERT_THAT( QString::compare( log_data.getExpandedLines( 0, 3 ).at( 2 ), QStringLiteral( "COMÉDIE" ) ), 0 );
}
