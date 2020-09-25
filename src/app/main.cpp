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

#include <QFileInfo>

#include <memory>

#include <iostream>

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

#include <QtCore/QJsonDocument>

#include <cli11/cli11.hpp>

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

int main( int argc, char* argv[] )
{
    setApplicationAttributes();

    KloggApp app( argc, argv );

    // Configuration

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

    LOG( logINFO ) << "Klogg instance " << app.instanceId();

    QPluginLoader vistaStyleLoader{ "styles/qwindowsvistastyle.dll" };
    vistaStyleLoader.load();
    LOG( logINFO ) << "Loading qwindowsvistastyle: " << vistaStyleLoader.errorString();

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

    return app.exec();
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
