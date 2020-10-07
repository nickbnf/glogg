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

#include "klogg_version.h"
#include "log.h"
#include "openfilehelper.h"

#include <QByteArray>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextEdit>
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

QJsonDocument extractJson( const QByteArray& data, int& lastOffset )
{
    QJsonParseError parseError;
    auto envelopeJson = QJsonDocument::fromJson( data, &parseError );
    if ( parseError.error != QJsonParseError::NoError ) {
        envelopeJson = QJsonDocument::fromJson( data.mid( 0, parseError.offset ) );
    }

    lastOffset = parseError.offset;
    return envelopeJson;
}

std::vector<QJsonDocument> extractMessage( const QByteArray& envelopeString )
{
    std::vector<QJsonDocument> messages;
    int position = 0;
    auto offset = 0;
    do {
        messages.push_back( extractJson( envelopeString.mid( position ), offset ) );
        position += offset;
    } while ( offset > 0 && position < envelopeString.size() );

    return messages;
}

QDialog::DialogCode askUserConfirmation( const QString& formattedReport, const QString& reportPath )
{
    auto message = std::make_unique<QLabel>();
    message->setText( "During last run application has encountered an unexpected error." );

    auto crashReportHeader = std::make_unique<QLabel>();
    crashReportHeader->setText( "We collected the following crash report:" );

    auto report = std::make_unique<QTextEdit>();
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
                      [&reportPath] { showPathInFileExplorer( reportPath ); } );

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

int askUserConfirmation( sentry_envelope_t* envelope, void* )
{
    size_t size_out = 0;
    char* rawEnvelope = sentry_envelope_serialize( envelope, &size_out );
    auto envelopeString = QByteArray( rawEnvelope );
    sentry_free( rawEnvelope );

    const auto messages = extractMessage( envelopeString );
    QString formattedReport;
    for ( const auto& message : messages ) {
        formattedReport.append( QString::fromUtf8( message.toJson( QJsonDocument::Indented ) ) );
    }

    return askUserConfirmation( formattedReport, sentryDatabasePath().append( "/last_crash" ) )
                   == QDialog::Accepted
               ? 0
               : 1;
}

void checkCrashpadReports( const QString& databasePath )
{
    using namespace crashpad;

#ifdef Q_OS_WIN
    auto database = CrashReportDatabase::InitializeWithoutCreating(
        base::FilePath{ databasePath.toStdWString() } );
#else
    auto database = CrashReportDatabase::InitializeWithoutCreating(
        base::FilePath{ databasePath.toStdString() } );
#endif

    std::vector<CrashReportDatabase::Report> pendingReports;
    database->GetCompletedReports( &pendingReports );
    LOG( logINFO ) << "Pending reports " << pendingReports.size();

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
            }
            else {
                database->DeleteReport( report.uuid );
            }
        }
    }
}
} // namespace

CrashHandler::CrashHandler()
{
    sentry_options_t* sentryOptions = sentry_options_new();

    sentry_options_set_logger( sentryOptions, logSentry, nullptr );
    sentry_options_set_debug( sentryOptions, 1 );

    const auto dumpPath = sentryDatabasePath();

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

    auto transport = sentry_transport_new_default();

    sentry_transport_set_ask_consent_func( transport, askUserConfirmation );

    sentry_options_set_transport( sentryOptions, transport );

    sentry_init( sentryOptions );

    sentry_set_tag( "commit", kloggCommit().data() );
    sentry_set_tag( "qt", qVersion() );

    checkCrashpadReports( dumpPath );
}

CrashHandler::~CrashHandler()
{
    sentry_shutdown();
}
