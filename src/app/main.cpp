/*
 * Copyright (C) 2009, 2010, 2011, 2013, 2014 Nicolas Bonnefon and other contributors
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

/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov and other contributors
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

#ifdef KLOGG_USE_TBB_MALLOC
#include <tbb/tbbmalloc_proxy.h>
#endif

#include <iostream>
#include <memory>

#include <cli11/cli11.hpp>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include "kloggapp.h"

#include "configuration.h"
#include "mainwindow.h"
#include "persistentinfo.h"
#include "recentfiles.h"
#include "savedsearches.h"
#include "versionchecker.h"

#include "klogg_version.h"

#include <QFileInfo>
#include <QtCore/QJsonDocument>

#include "sentry.h"

#ifdef KLOGG_PORTABLE
const bool PersistentInfo::forcePortable = true;
#else
const bool PersistentInfo::forcePortable = false;
#endif

static void print_version();

void setApplicationAttributes()
{
    // When QNetworkAccessManager is instantiated it regularly starts polling
    // all network interfaces to see if anything changes and if so, what. This
    // creates a latency spike every 10 seconds on Mac OS 10.12+ and Windows 7 >=
    // when on a wifi connection.
    // So here we disable it for lack of better measure.
    // This will also cause this message: QObject::startTimer: Timers cannot
    // have negative intervals
    // For more info see:
    // - https://bugreports.qt.io/browse/QTBUG-40332
    // - https://bugreports.qt.io/browse/QTBUG-46015
    qputenv( "QT_BEARER_POLL_TIMEOUT", QByteArray::number( -1 ) );

    const auto& config = Configuration::getSynced();

    if ( config.enableQtHighDpi() ) {
        // This attribute must be set before QGuiApplication is constructed:
        QCoreApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
        // We support high-dpi (aka Retina) displays
        QCoreApplication::setAttribute( Qt::AA_UseHighDpiPixmaps );

#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
            static_cast<Qt::HighDpiScaleFactorRoundingPolicy>( config.scaleFactorRounding() ) );
#endif
    }
    else {
        QCoreApplication::setAttribute( Qt::AA_DisableHighDpiScaling );
    }

    QCoreApplication::setAttribute( Qt::AA_DontShowIconsInMenus );

    if ( config.enableQtHighDpi() ) {
    }

    // fractional scaling

#ifdef Q_OS_WIN
    QCoreApplication::setAttribute( Qt::AA_DisableWindowContextHelpButton );

#endif
}

struct CliParameters {
    bool new_session = false;
    bool load_session = false;
    bool multi_instance = false;
    bool log_to_file = false;
    bool follow_file = false;
    int64_t log_level = static_cast<int64_t>( logWARNING );

    std::vector<QString> filenames;

    CliParameters() = default;

    CliParameters( CLI::App& options, int argc, char* const* argv )
    {
        options.add_flag_function(
            "-v,--version",
            []( auto ) {
                print_version();
                exit( 0 );
            },
            "print version information" );

        options.add_flag(
            "-m,--multi", multi_instance,
            "allow multiple instance of klogg to run simultaneously (use together with -s)" );
        options.add_flag( "-s,--load-session", load_session,
                          "load the previous session (default when no file is passed)" );
        options.add_flag( "-n,--new-session", new_session,
                          "do not load the previous session (default when a file is passed)" );

        options.add_flag( "-l,--log", log_to_file, "save the log to a file" );

        options.add_flag( "-f,--follow", follow_file, "follow initial opened files" );

        options.add_flag_function(
            "-d,--debug",
            [this]( auto count ) { log_level = static_cast<int64_t>( logWARNING ) + count; },
            "output more debug (include multiple times for more verbosity e.g. -dddd)" );

        std::vector<std::string> raw_filenames;
        options.add_option( "files", raw_filenames, "files to open" );

        options.parse( argc, argv );

        for ( const auto& file : raw_filenames ) {
            auto decodedName = QFile::decodeName( file.c_str() );
            if ( !decodedName.isEmpty() ) {
                const auto fileInfo = QFileInfo( decodedName );
                filenames.emplace_back( fileInfo.absoluteFilePath() );
            }
        }
    }
};

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

void initializeSentry()
{
    sentry_options_t* sentryOptions = sentry_options_new();

    const auto dumpPath = QDir::temp().filePath( "klogg_dump" );

#ifdef Q_OS_WIN
    sentry_options_set_database_pathw( sentryOptions, dumpPath.toStdWString().c_str() );
#else
    sentry_options_set_database_path( sentryOptions, dumpPath.toStdString().c_str() );
#endif

    sentry_options_set_dsn(
        sentryOptions,
        "https://aad3b270e5ba4ec2915eb5caf6e6d929@o453796.ingest.sentry.io/5442855" );

    sentry_options_set_require_user_consent( sentryOptions, false );
    sentry_options_set_auto_session_tracking( sentryOptions, false );

    sentry_options_set_logger( sentryOptions, logSentry, nullptr );
    sentry_options_set_debug( sentryOptions, 1 );

    sentry_options_set_symbolize_stacktraces( sentryOptions, true );

    sentry_options_set_environment( sentryOptions, "development" );
    sentry_options_set_release( sentryOptions, kloggVersion().data() );

    auto transport = sentry_transport_new_default();

    sentry_transport_set_ask_consent_func( transport, []( sentry_envelope_t* envelope, void* ) {
        size_t size_out = 0;
        char* s = sentry_envelope_serialize( envelope, &size_out );
        LOG( logINFO ) << "Envelope: " << s;
        sentry_free( s );

        const auto userAction = QMessageBox::question(
            nullptr, "klogg - Oops",
            QString( "During last run klogg has encountered an unexpected error. "
                     "Upload debug data to developers?" ),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

        return userAction == QMessageBox::Yes ? 0 : 1;
    } );

    sentry_options_set_transport( sentryOptions, transport );

    sentry_init( sentryOptions );

    sentry_set_tag( "commit", kloggCommit().data() );
    sentry_set_tag( "qt", qVersion() );
}

int main( int argc, char* argv[] )
{
    setApplicationAttributes();

    KloggApp app( argc, argv );

    CliParameters parameters;
    CLI::App options{ "Klogg -- fast log explorer" };
    try {
        parameters = CliParameters( options, argc, argv );
    } catch ( const CLI::ParseError& e ) {
        return options.exit( e );
    } catch ( const std::exception& e ) {
        std::cerr << "Exception of unknown type: " << e.what() << std::endl;
    }

    app.initLogger( static_cast<plog::Severity>( parameters.log_level ), parameters.log_to_file );

    initializeSentry();

    LOG( logINFO ) << "Klogg instance " << app.instanceId();

    if ( !parameters.multi_instance && app.isSecondary() ) {
        LOG( logINFO ) << "Found another klogg, pid " << app.primaryPid();
        app.sendFilesToPrimaryInstance( parameters.filenames );
    }
    else {
        Configuration::getSynced();

        // Load the existing session if needed
        const auto& config = Configuration::get();
        plog::EnableLogging( config.enableLogging(), config.loggingLevel() );

        MainWindow* mw = nullptr;
        if ( parameters.load_session
             || ( parameters.filenames.empty() && !parameters.new_session
                  && config.loadLastSession() ) ) {
            mw = app.reloadSession();
        }
        else {
            mw = app.newWindow();
            mw->reloadGeometry();
            LOG( logDEBUG ) << "MainWindow created.";
            mw->show();
        }

        for ( const auto& filename : parameters.filenames ) {
            mw->loadInitialFile( filename, parameters.follow_file );
        }

        app.startBackgroundTasks();
    }

    const auto resultCode = app.exec();

    sentry_shutdown();

    return resultCode;
}

static void print_version()
{
    std::cout << "klogg " << kloggVersion().data() << "\n";
    std::cout << "Built " << kloggBuildDate().data() << " from " << kloggCommit().data() << "("
              << kloggGitVersion().data() << ")\n";

    std::cout << "Copyright (C) 2020 Nicolas Bonnefon, Anton Filimonov and other contributors\n";
    std::cout << "This is free software.  You may redistribute copies of it under the terms of\n";
    std::cout << "the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n";
    std::cout << "There is NO WARRANTY, to the extent permitted by law.\n";
}
