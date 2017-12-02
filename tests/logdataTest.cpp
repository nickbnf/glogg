#include <iostream>

#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QProcess>
#include <QThread>

#include "log.h"
#include "test_utils.h"
#include "file_write_helper.h"

#include "data/logdata.h"


static const qint64 SL_NB_LINES = 500LL;
static const qint64 VBL_NB_LINES = 5000LL;

namespace {

	class WriteFileThread : public QThread
	{
		Q_OBJECT
	public:
		WriteFileThread(QFile* file, int numberOfLines = 200, WriteFileModification flag = WriteFileModification::None)
			: file_{ file }, numberOfLines_{numberOfLines}, flag_{flag}
		{}

	protected:
		void run() override
		{
			QString writeHelper = QCoreApplication::applicationDirPath() + QDir::separator() + QLatin1Literal("file_write_helper");
			QStringList arguments;
			arguments << file_->fileName() << QString::number(numberOfLines_) << QString::number(static_cast<uint8_t>(flag_));

			const auto result = QProcess::execute(writeHelper, arguments);
			LOG(logINFO) << "Write helper result " << result;
		}

	private:
		QFile* file_;
		int numberOfLines_;
		WriteFileModification flag_;

	};

#include "logdataTest.moc"


	void writeDataToFile(QFile& file, int numberOfLines = 200, WriteFileModification flag = WriteFileModification::None) {
		auto thread = new WriteFileThread(&file, numberOfLines, flag);
		thread->start();
		thread->wait();
		thread->deleteLater();
	}

	void writeDataToFileBackground(QFile& file, int numberOfLines = 200, WriteFileModification flag = WriteFileModification::None) {
		auto thread = new WriteFileThread(&file, numberOfLines, flag);
		thread->start();
		QObject::connect(thread, &WriteFileThread::finished, thread, &WriteFileThread::deleteLater);
	}

}


class LogDataChanging : public testing::Test {
protected:
	QTemporaryDir tempDir;
};

TEST_F( LogDataChanging, changingFile ) {
    LogData log_data;

    SafeQSignalSpy finishedSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );
    SafeQSignalSpy progressSpy( &log_data, SIGNAL( loadingProgressed( int ) ) );
    SafeQSignalSpy changedSpy( &log_data,
            SIGNAL( fileChanged( LogData::MonitoredFileStatus ) ) );

    // Generate a small file
	QFile file { tempDir.path() + QDir::separator() + QLatin1Literal("testlog.txt") };
    if ( file.open( QIODevice::ReadWrite | QIODevice::Truncate ) ) {
        writeDataToFile( file );
    }

    // Start loading it
    log_data.attachFile( file.fileName() );

    // and wait for the signal
    ASSERT_TRUE( finishedSpy.safeWait() );

    // Check we have the small file
    ASSERT_THAT( finishedSpy.count(), 1 );
    ASSERT_THAT( log_data.getNbLine().get(), 200LL );
    ASSERT_THAT( log_data.getMaxLength().get(), SL_LINE_LENGTH );
    ASSERT_THAT( log_data.getFileSize(), 200 * (SL_LINE_LENGTH+1LL) );

#ifndef _WIN32
	auto finishedSpyCount = finishedSpy.count();
    // Add some data to it
    if ( file.isOpen() ) {
		// To test the edge case when the final line is not complete
#ifdef _WIN32
		writeDataToFileBackground( file, 200, WriteFileModification::EndWithPartialLineBegin );
#else
		writeDataToFile(file, 200, WriteFileModification::EndWithPartialLineBegin);
#endif
    }

    // and wait for the signals
    finishedSpy.wait( 10000 );

	ASSERT_GE( finishedSpy.count(), finishedSpyCount );

    // Check we have a bigger file
    ASSERT_GE( changedSpy.count(), 1 );
    ASSERT_THAT( finishedSpy.count(), 2 );
    ASSERT_THAT( log_data.getNbLine().get(), 401LL );
    ASSERT_THAT( log_data.getMaxLength().get(), SL_LINE_LENGTH );
    ASSERT_THAT( log_data.getFileSize(), (qint64) (400 * (SL_LINE_LENGTH+1LL)
            + strlen( partial_line_begin ) ) );

    {
        SafeQSignalSpy finishedSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

        // Add a couple more lines, including the end of the unfinished one.
        if ( file.isOpen() ) {
#ifdef _WIN32
			writeDataToFileBackground( file, 20, WriteFileModification::StartWithPartialLineEnd );
#else
			writeDataToFile(file, 20, WriteFileModification::StartWithPartialLineEnd);
#endif
        }

        // and wait for the signals
        ASSERT_TRUE( finishedSpy.wait( 10000 ) );

        // Check we have a bigger file
        ASSERT_GE( changedSpy.count(), 2 );
        ASSERT_THAT( finishedSpy.count(), 1 );
        ASSERT_THAT( log_data.getNbLine().get(), 421LL );
        ASSERT_THAT( log_data.getMaxLength().get(), SL_LINE_LENGTH );
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
        ASSERT_THAT( log_data.getNbLine().get(), 0LL );
        ASSERT_THAT( log_data.getMaxLength().get(), 0 );
        ASSERT_THAT( log_data.getFileSize(), 0LL );
    }
#endif
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

    ASSERT_THAT( log_data.getNbLine().get(), 0LL );
    ASSERT_THAT( log_data.getMaxLength().get(), 0 );
    ASSERT_THAT( log_data.getFileSize(), 0LL );
}

TEST_F( LogDataBehaviour, cannotBeReAttached ) {
    LogData log_data;
    SafeQSignalSpy endSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

    log_data.attachFile( smallFile_.fileName() );
    endSpy.safeWait( 10000 );

    ASSERT_THROW( log_data.attachFile( bigFile_.fileName() ), CantReattachErr );
}
