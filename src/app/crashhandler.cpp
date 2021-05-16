/*
 * Copyright (C) 2020 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "crashhandler.h"

#include "sentry.h"

#ifdef KLOGG_USE_MIMALLOC
#include <mimalloc.h>
#endif

#include "klogg_version.h"
#include "log.h"
#include "memory_info.h"
#include "openfilehelper.h"

#include <QByteArray>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>

#include "client/crash_report_database.h"

namespace {

constexpr const char* DSN
    = "https://aad3b270e5ba4ec2915eb5caf6e6d929@o453796.ingest.sentry.io/5442855";

QString sentryDatabasePath()
{
#ifdef KLOGG_PORTABLE
    auto basePath = QCoreApplication::applicationDirPath();
#else
    auto basePath = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
#endif

    return basePath.append( "/klogg_dump" );
}

void logSentry( sentry_level_t level, const char* message, va_list args, void* userdata )
{
    Q_UNUSED( userdata );
    plog::Severity severity;
    switch ( level ) {
    case SENTRY_LEVEL_WARNING:
        severity = plog::Severity::warning;
        break;
    case SENTRY_LEVEL_ERROR:
        severity = plog::Severity::error;
        break;
    default:
        severity = plog::Severity::info;
        break;
    }

    PLOG( severity ).printf( message, args );
}

QDialog::DialogCode askUserConfirmation( const QString& formattedReport, const QString& reportPath )
{
    auto message = std::make_unique<QLabel>();
    message->setText( "During last run application has encountered an unexpected error." );

    auto crashReportHeader = std::make_unique<QLabel>();
    crashReportHeader->setText( "We collected the following crash report:" );

    auto report = std::make_unique<QPlainTextEdit>();
    report->setReadOnly( true );
    report->setPlainText( formattedReport );
    report->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    auto sendReportLabel = std::make_unique<QLabel>();
    sendReportLabel->setText( "Application can send this report to sentry.io for developers to "
                              "analyze and fix the issue" );

    auto privacyPolicy = std::make_unique<QLabel>();
    privacyPolicy->setText(
        "<a href=\"https://klogg.filimonov.dev/docs/privacy_policy\">Privacy policy</a>" );

    privacyPolicy->setTextFormat( Qt::RichText );
    privacyPolicy->setTextInteractionFlags( Qt::TextBrowserInteraction );
    privacyPolicy->setOpenExternalLinks( true );

    auto exploreButton = std::make_unique<QPushButton>();
    exploreButton->setText( "Open report directory" );
    exploreButton->setFlat( true );
    QObject::connect( exploreButton.get(), &QPushButton::clicked,
                      [ &reportPath ] { showPathInFileExplorer( reportPath ); } );

    auto privacyLayout = std::make_unique<QHBoxLayout>();
    privacyLayout->addWidget( privacyPolicy.release() );
    privacyLayout->addStretch();
    privacyLayout->addWidget( exploreButton.release() );

    auto buttonBox = std::make_unique<QDialogButtonBox>();
    buttonBox->addButton( "Send report", QDialogButtonBox::AcceptRole );
    buttonBox->addButton( "Discard report", QDialogButtonBox::RejectRole );

    auto confirmationDialog = std::make_unique<QDialog>();
    confirmationDialog->resize( 800, 600 );

    QObject::connect( buttonBox.get(), &QDialogButtonBox::accepted, confirmationDialog.get(),
                      &QDialog::accept );
    QObject::connect( buttonBox.get(), &QDialogButtonBox::rejected, confirmationDialog.get(),
                      &QDialog::reject );

    auto layout = std::make_unique<QVBoxLayout>();
    layout->addWidget( message.release() );
    layout->addWidget( crashReportHeader.release() );
    layout->addWidget( report.release() );
    layout->addWidget( sendReportLabel.release() );
    layout->addLayout( privacyLayout.release() );
    layout->addWidget( buttonBox.release() );

    confirmationDialog->setLayout( layout.release() );

    return static_cast<QDialog::DialogCode>( confirmationDialog->exec() );
}

void reportIssue( const std::string& reportUuid )
{
    const QString version = kloggVersion();
    const QString commit = kloggCommit();

    const QString os = QSysInfo::prettyProductName();
    const QString kernelType = QSysInfo::kernelType();
    const QString kernelVersion = QSysInfo::kernelVersion();
    const QString arch = QSysInfo::currentCpuArchitecture();
    const QString built_for = QSysInfo::buildAbi();

    const QString body = QString( "Details for the issue\n"
                                  "--------------------\n\n"
                                  "#### What did you do?\n\n\n"

                                  "-------------------------\n"
                                  "Crash id\n"
                                  "%1\n"
                                  "-------------------------\n"
                                  "Useful extra information\n"
                                  "-------------------------\n"
                                  "> Klogg version %2 (built from commit %3) [built for %4]\n"
                                  "> running on %5 (%6/%7) [%8]\n"
                                  "> and Qt %9" )
                             .arg( reportUuid.c_str(), version, commit, built_for, os, kernelType,
                                   kernelVersion, arch, qVersion() );

    QUrlQuery query;
    query.addQueryItem( "labels", "type: crash" );
    query.addQueryItem( "body", body );

    QUrl url( "https://github.com/variar/klogg/issues/new" );
    url.setQuery( query );
    QDesktopServices::openUrl( url );
}

bool checkCrashpadReports( const QString& databasePath )
{
    using namespace crashpad;

    bool needWaitForUpload = false;

#ifdef Q_OS_WIN
    auto database = CrashReportDatabase::InitializeWithoutCreating(
        base::FilePath{ databasePath.toStdWString() } );
#else
    auto database = CrashReportDatabase::InitializeWithoutCreating(
        base::FilePath{ databasePath.toStdString() } );
#endif

    std::vector<CrashReportDatabase::Report> pendingReports;
    database->GetCompletedReports( &pendingReports );
    LOG_INFO << "Pending reports " << pendingReports.size();

#ifdef Q_OS_WIN
    const auto stackwalker = QCoreApplication::applicationDirPath() + "/klogg_minidump_dump.exe";
#else
    const auto stackwalker = QCoreApplication::applicationDirPath() + "/klogg_minidump_dump";
#endif

    for ( const auto& report : pendingReports ) {
        if ( !report.uploaded ) {
#ifdef Q_OS_WIN
            const auto reportFile = QString::fromStdWString( report.file_path.value() );

#else
            const auto reportFile = QString::fromStdString( report.file_path.value() );
#endif

            QProcess stackProcess;
            stackProcess.start( stackwalker, QStringList() << reportFile );
            stackProcess.waitForFinished();

            QString formattedReport = reportFile;
            formattedReport.append( QChar::LineFeed )
                .append( QString::fromUtf8( stackProcess.readAllStandardOutput() ) );

            if ( QDialog::Accepted == askUserConfirmation( formattedReport, reportFile ) ) {
                database->RequestUpload( report.uuid );
                needWaitForUpload = true;
            }
            else {
                database->DeleteReport( report.uuid );
            }

            if ( QMessageBox::Yes
                 == QMessageBox::question( nullptr, "Klogg", "Create issue on GitHub",
                                           QMessageBox::Yes, QMessageBox::No ) ) {
                reportIssue( report.uuid.ToString() );
            }
        }
    }
    return needWaitForUpload;
}
} // namespace

CrashHandler::CrashHandler()
{
    const auto dumpPath = sentryDatabasePath();
    const auto hasDumpDir = QDir{ dumpPath }.mkpath( "." );

    const auto needWaitForUpload = hasDumpDir ? checkCrashpadReports( dumpPath ) : false;

    sentry_options_t* sentryOptions = sentry_options_new();

    sentry_options_set_logger( sentryOptions, logSentry, nullptr );
    sentry_options_set_debug( sentryOptions, 1 );

#ifdef Q_OS_WIN
    const auto handlerPath = QCoreApplication::applicationDirPath() + "/klogg_crashpad_handler.exe";
    sentry_options_set_database_pathw( sentryOptions, dumpPath.toStdWString().c_str() );
    sentry_options_set_handler_pathw( sentryOptions, handlerPath.toStdWString().c_str() );
#else
    const auto handlerPath = QCoreApplication::applicationDirPath() + "/klogg_crashpad_handler";
    sentry_options_set_database_path( sentryOptions, dumpPath.toStdString().c_str() );
    sentry_options_set_handler_path( sentryOptions, handlerPath.toStdString().c_str() );
#endif

    sentry_options_set_dsn( sentryOptions, DSN );

    // klogg asks confirmation and sends reports using crashpad
    sentry_options_set_require_user_consent( sentryOptions, true );

    sentry_options_set_auto_session_tracking( sentryOptions, false );

    sentry_options_set_symbolize_stacktraces( sentryOptions, true );

    sentry_options_set_environment( sentryOptions, "development" );
    sentry_options_set_release( sentryOptions, kloggVersion().data() );

    sentry_init( sentryOptions );

    sentry_set_tag( "commit", kloggCommit().data() );
    sentry_set_tag( "qt", qVersion() );
    sentry_set_tag( "build_arch", QSysInfo::buildCpuArchitecture().toLatin1().data() );

    const auto totalMemory = std::to_string( physicalMemory() );
    LOG_INFO << "Physical memory " << totalMemory;

    sentry_set_extra( "memory", sentry_value_new_string( totalMemory.c_str() ) );

    memoryUsageTimer_ = std::make_unique<QTimer>();
    QObject::connect( memoryUsageTimer_.get(), &QTimer::timeout, []() {
        const auto vmUsed = std::to_string( usedMemory() );

        sentry_set_extra( "vm_used", sentry_value_new_string( vmUsed.c_str() ) );

#ifdef KLOGG_USE_MIMALLOC
        size_t elapsedMsecs, userMsecs, systemMsecs, currentRss, peakRss, currentCommit, peakCommit,
            pageFaults;
        mi_process_info( &elapsedMsecs, &userMsecs, &systemMsecs, &currentRss, &peakRss,
                         &currentCommit, &peakCommit, &pageFaults );
        sentry_set_extra( "elapsed_msecs",
                          sentry_value_new_string( std::to_string( elapsedMsecs ).c_str() ) );
        sentry_set_extra( "user_msecs",
                          sentry_value_new_string( std::to_string( userMsecs ).c_str() ) );
        sentry_set_extra( "system_msecs",
                          sentry_value_new_string( std::to_string( systemMsecs ).c_str() ) );
        sentry_set_extra( "current_rss",
                          sentry_value_new_string( std::to_string( currentRss ).c_str() ) );
        sentry_set_extra( "peak_rss",
                          sentry_value_new_string( std::to_string( peakRss ).c_str() ) );
        sentry_set_extra( "current_commit",
                          sentry_value_new_string( std::to_string( currentCommit ).c_str() ) );
        sentry_set_extra( "peak_commit",
                          sentry_value_new_string( std::to_string( peakCommit ).c_str() ) );
        sentry_set_extra( "page_faults",
                          sentry_value_new_string( std::to_string( pageFaults ).c_str() ) );

        LOG_INFO << "Process stats, vm_used " << vmUsed << ", elapsed_msecs " << elapsedMsecs
                 << ", user_msecs " << userMsecs << ", system_msecs " << systemMsecs
                 << ", current_rss " << currentRss << ", peak_rss " << peakRss
                 << ", current_commit " << currentCommit << ", peak_commit " << peakCommit
                 << ", page_faults " << pageFaults;
#else
        LOG_INFO << "Process stats, vm_used " << vmUsed;
#endif
    } );
    memoryUsageTimer_->start( 10000 );

    if ( needWaitForUpload ) {
        QProgressDialog progressDialog;
        progressDialog.setLabelText( QString( "Uploading crash reports" ) );
        progressDialog.setRange( 0, 0 );

        QTimer::singleShot( 30 * 1000, &progressDialog, &QProgressDialog::cancel );
        progressDialog.exec();
    }
}

CrashHandler::~CrashHandler()
{
    memoryUsageTimer_->stop();
    sentry_shutdown();
}
