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

#include <QFileInfo>

#include <memory>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <iomanip>
using namespace std;

#ifdef _WIN32
#include "unistd.h"
#endif

#include "gloggapp.h"
#include "persistentinfo.h"
#include "sessioninfo.h"
#include "configuration.h"
#include "filterset.h"
#include "recentfiles.h"
#include "session.h"
#include "mainwindow.h"
#include "savedsearches.h"
#include "loadingstatus.h"

#include "externalcom.h"

#ifdef GLOGG_SUPPORTS_DBUS
#include "dbusexternalcom.h"
#elif GLOGG_SUPPORTS_SOCKETIPC
#include "socketexternalcom.h"
#endif


#include "log.h"

static void print_version();

int main(int argc, char *argv[])
{
    GloggApp app(argc, argv);

    vector<string> filenames;

    // Configuration
    bool new_session = false;
    bool load_session = false;
    bool multi_instance = false;
#ifdef _WIN32
    bool log_to_file = false;
#endif

    TLogLevel logLevel = logWARNING;

    try {
        po::options_description desc("Usage: glogg [options] [files]");
        desc.add_options()
            ("help,h", "print out program usage (this message)")
            ("version,v", "print glogg's version information")
            ("multi,m", "allow multiple instance of glogg to run simultaneously (use together with -s)")
            ("load-session,s", "load the previous session (default when no file is passed)")
            ("new-session,n", "do not load the previous session (default when a file is passed)")
#ifdef _WIN32
            ("log,l", "save the log to a file (Windows only)")
#endif
            ("debug,d", "output more debug (include multiple times for more verbosity e.g. -dddd)")
            ;
        po::options_description desc_hidden("Hidden options");
        // For -dd, -ddd...
        for ( string s = "dd"; s.length() <= 10; s.append("d") )
            desc_hidden.add_options()(s.c_str(), "debug");

        desc_hidden.add_options()
            ("input-file", po::value<vector<string>>(), "input file")
            ;

        po::options_description all_options("all options");
        all_options.add(desc).add(desc_hidden);

        po::positional_options_description positional;
        positional.add("input-file", -1);

        int command_line_style = (((po::command_line_style::unix_style ^
                po::command_line_style::allow_guessing) |
                po::command_line_style::allow_long_disguise) ^
                po::command_line_style::allow_sticky);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                options(all_options).
                positional(positional).
                style(command_line_style).run(),
                vm);
        po::notify(vm);

        if ( vm.count("help") ) {
            desc.print(cout);
            return 0;
        }

        if ( vm.count("version") ) {
            print_version();
            return 0;
        }

        if ( vm.count( "debug" ) )
            logLevel = logINFO;

        if ( vm.count( "multi" ) )
            multi_instance = true;

        if ( vm.count( "new-session" ) )
            new_session = true;

        if ( vm.count( "load-session" ) )
            load_session = true;

#ifdef _WIN32
        if ( vm.count( "log" ) )
            log_to_file = true;
#endif

        for ( string s = "dd"; s.length() <= 10; s.append("d") )
            if ( vm.count( s ) )
                logLevel = (TLogLevel) (logWARNING + s.length());

        if ( vm.count("input-file") ) {
            filenames = vm["input-file"].as<vector<string>>();
        }
    }
    catch(exception& e) {
        cerr << "Option processing error: " << e.what() << endl;
        return 1;
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";
    }

#ifdef _WIN32
    if ( log_to_file )
    {
        char file_name[255];
        snprintf( file_name, sizeof file_name, "glogg_%d.log", getpid() );
        FILE* file = fopen(file_name, "w");
        Output2FILE::Stream() = file;
    }
#endif

    FILELog::setReportingLevel( logLevel );

    for ( auto& filename: filenames ) {
        if ( ! filename.empty() ) {
            // Convert to absolute path
            QFileInfo file( QString::fromLocal8Bit( filename.c_str() ) );
            filename = file.absoluteFilePath().toStdString();
            LOG( logDEBUG ) << "Filename: " << filename;
        }
    }

    // External communicator
    shared_ptr<ExternalCommunicator> externalCommunicator = nullptr;
    shared_ptr<ExternalInstance> externalInstance = nullptr;

    try {
#ifdef GLOGG_SUPPORTS_DBUS
        externalCommunicator = make_shared<DBusExternalCommunicator>();
        externalInstance = shared_ptr<ExternalInstance>(
                externalCommunicator->otherInstance() );
#elif GLOGG_SUPPORTS_SOCKETIPC
        externalCommunicator = make_shared<SocketExternalCommunicator>();
        auto ptr = externalCommunicator->otherInstance();
        externalInstance = shared_ptr<ExternalInstance>( ptr );
#endif
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
            externalInstance->loadFile( QString::fromStdString( filename ) );
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
    std::shared_ptr<Configuration> config =
        Persistent<Configuration>( "settings" );
    GetPersistentInfo().retrieve( *config );

    std::unique_ptr<Session> session( new Session() );
    MainWindow mw( std::move( session ), externalCommunicator );

    // Geometry
    mw.reloadGeometry();

    // Load the existing session if needed
    if ( load_session || ( filenames.empty() && !new_session && config->loadLastSession() ) )
        mw.reloadSession();

    LOG(logDEBUG) << "MainWindow created.";
    mw.show();

    for ( const auto& filename: filenames ) {
        mw.loadInitialFile( QString::fromStdString( filename ) );
    }

    mw.startBackgroundTasks();

    return app.exec();
}

static void print_version()
{
    cout << "glogg " GLOGG_VERSION "\n";
#ifdef GLOGG_COMMIT
    cout << "Built " GLOGG_DATE " from " GLOGG_COMMIT "\n";
#endif
    cout << "Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014, 2015 Nicolas Bonnefon and other contributors\n";
    cout << "This is free software.  You may redistribute copies of it under the terms of\n";
    cout << "the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n";
    cout << "There is NO WARRANTY, to the extent permitted by law.\n";
}
