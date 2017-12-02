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

#include <QApplication>
#include <QFileInfo>

#include <memory>

#include <iostream>
#include <iomanip>
using namespace std;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include "persistentinfo.h"
#include "sessioninfo.h"
#include "configuration.h"
#include "filterset.h"
#include "recentfiles.h"
#include "session.h"
#include "mainwindow.h"
#include "savedsearches.h"
#include "data/loadingstatus.h"

#include "externalcom.h"

#ifdef GLOGG_SUPPORTS_DBUS
#include "dbusexternalcom.h"
#elif GLOGG_SUPPORTS_SOCKETIPC
#include "socketexternalcom.h"
#endif

#include "log.h"

#include "cli11/cli11.hpp"

#include "version.h"

#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>

static void print_version();

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<LinesCount>( "LinesCount" );
    qRegisterMetaType<LineNumber>( "LineNumber" );
    qRegisterMetaType<LineLength>( "LineLength" );

    vector<QString> filenames;

    // Configuration
    bool new_session = false;
    bool load_session = false;
    bool multi_instance = false;
    bool log_to_file = false;

    auto logLevel = logWARNING;
    CLI::App options{ "Klogg -- fast log explorer" };
    try {
        options.add_flag_function( "-v,--version", []( size_t ){ print_version(); exit( 0 ); }, "print version information" );

        options.add_flag( "-m,--multi", multi_instance, "allow multiple instance of klogg to run simultaneously (use together with -s)" );
        options.add_flag( "-s,--load-session", load_session, "load the previous session (default when no file is passed)" );
        options.add_flag( "-n,--new-session", new_session, "do not load the previous session (default when a file is passed)" );


        options.add_flag( "-l,--log", log_to_file, "save the log to a file" );

        options.add_flag_function( "-d,--debug",
                         [ &logLevel ]( size_t count ) { logLevel = static_cast<std::decay<decltype(logLevel)>::type>( logWARNING + count ); },
                        "output more debug (include multiple times for more verbosity e.g. -dddd)"
        );

        vector<string> files;
        options.add_option( "files", files, "files to open" );

        options.parse( argc, argv );

        for ( const auto& file: files ) {
            filenames.push_back( QFile::decodeName( file.c_str() ) );
        }
    }
    catch ( const CLI::ParseError &e ) {
       return options.exit( e );
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";
    }

    std::unique_ptr<plog::IAppender> logAppender;

    if ( log_to_file ) {
        char file_name[255];
        snprintf( file_name, sizeof file_name, "klogg_%lld.log", app.applicationPid() );
        logAppender = std::make_unique<plog::RollingFileAppender<plog::GloggFormatter>>( file_name );
    }
    else {
        logAppender = std::make_unique<plog::ColorConsoleAppender<plog::GloggFormatter>>();
    }

    plog::init(logLevel, logAppender.get());

    for ( auto& filename: filenames ) {
        if ( ! filename.isEmpty() ) {
            // Convert to absolute path
            const QFileInfo file( filename );
            filename = file.absoluteFilePath();
            LOG( logDEBUG ) << "Filename: " << filename.toStdString();
        }
    }

    // External communicator
    shared_ptr<ExternalCommunicator> externalCommunicator = nullptr;
    shared_ptr<ExternalInstance> externalInstance = nullptr;

    try {
#ifdef GLOGG_SUPPORTS_DBUS
        externalCommunicator = make_shared<DBusExternalCommunicator>();
#elif GLOGG_SUPPORTS_SOCKETIPC
        externalCommunicator = make_shared<SocketExternalCommunicator>();
#endif
        if (externalCommunicator) {
          auto ptr = externalCommunicator->otherInstance();
          externalInstance = shared_ptr<ExternalInstance>( ptr );
        }
    }
    catch(CantCreateExternalErr& e) {
        LOG(logWARNING) << "Cannot initialise external communication.";
    }

    LOG(logDEBUG) << "externalInstance = " << externalInstance;
    if ( ( ! multi_instance ) && externalInstance ) {
        uint32_t version = externalInstance->getVersion();
        LOG(logINFO) << "Found another glogg (version = "
            << std::setbase(16) << version << ")";

        for ( const auto& filename: filenames ) {
            externalInstance->loadFile( filename );
        }

        return 0;
    }
    else {
        // FIXME: there is a race condition here. One glogg could start
        // between the declaration of externalInstance and here,
        // is it a problem?
        if ( externalCommunicator )
            externalCommunicator->startListening();
    }

    // Register types for Qt
    qRegisterMetaType<LoadingStatus>("LoadingStatus");

    // Register the configuration items
    GetPersistentInfo().migrateAndInit();
    GetPersistentInfo().registerPersistable(
            std::make_shared<SessionInfo>(), QString( "session" ) );
    GetPersistentInfo().registerPersistable(
            std::make_shared<Configuration>(), QString( "settings" ) );
    GetPersistentInfo().registerPersistable(
            std::make_shared<FilterSet>(), QString( "filterSet" ) );
    GetPersistentInfo().registerPersistable(
            std::make_shared<SavedSearches>(), QString( "savedSearches" ) );
    GetPersistentInfo().registerPersistable(
            std::make_shared<RecentFiles>(), QString( "recentFiles" ) );
#ifdef GLOGG_SUPPORTS_VERSION_CHECKING
    GetPersistentInfo().registerPersistable(
            std::make_shared<VersionCheckerConfig>(), QString( "versionChecker" ) );
#endif

#ifdef _WIN32
    // Allow the app to raise it's own windows (in case an external
    // glogg send us a file to open)
    AllowSetForegroundWindow(ASFW_ANY);
#endif

    // We support high-dpi (aka Retina) displays
    app.setAttribute( Qt::AA_UseHighDpiPixmaps );

    // No icon in menus
    app.setAttribute( Qt::AA_DontShowIconsInMenus );

    // FIXME: should be replaced by a two staged init of MainWindow
    GetPersistentInfo().retrieve( QString( "settings" ) );

    std::unique_ptr<Session> session( new Session() );
    MainWindow mw( std::move( session ), externalCommunicator );

    // Geometry
    mw.reloadGeometry();

    // Load the existing session if needed
    std::shared_ptr<Configuration> config =
        Persistent<Configuration>( "settings" );
    if ( load_session || ( filenames.empty() && !new_session && config->loadLastSession() ) )
        mw.reloadSession();

    LOG(logDEBUG) << "MainWindow created.";
    mw.show();

    for ( const auto& filename: filenames ) {
        mw.loadInitialFile( filename );
    }

    mw.startBackgroundTasks();

    return app.exec();
}

static void print_version()
{
    cout << "glogg " GLOGG_VERSION "\n";
#ifdef GLOGG_COMMIT
    cout << "Built " GLOGG_DATE " from " GLOGG_COMMIT "(" GLOGG_GIT_VERSION ")\n";
#endif
    cout << "Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014, 2015 Nicolas Bonnefon and other contributors\n";
    cout << "This is free software.  You may redistribute copies of it under the terms of\n";
    cout << "the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n";
    cout << "There is NO WARRANTY, to the extent permitted by law.\n";
}
