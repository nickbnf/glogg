/*
 * Copyright (C) 2009, 2010, 2011, 2013 Nicolas Bonnefon and other contributors
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

#include <memory>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
using namespace std;

#include "persistentinfo.h"
#include "sessioninfo.h"
#include "configuration.h"
#include "filterset.h"
#include "recentfiles.h"
#include "session.h"
#include "mainwindow.h"
#include "savedsearches.h"
#include "log.h"

static void print_version();

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    string filename = "";

    TLogLevel logLevel = logWARNING;

    try {
        po::options_description desc("Usage: glogg [options] [file]");
        desc.add_options()
            ("help,h", "print out program usage (this message)")
            ("version,v", "print glogg's version information")
            ("debug,d", "output more debug (include multiple times for more verbosity e.g. -dddd")
            ;
        po::options_description desc_hidden("Hidden options");
        // For -dd, -ddd...
        for ( string s = "dd"; s.length() <= 10; s.append("d") )
            desc_hidden.add_options()(s.c_str(), "debug");

        desc_hidden.add_options()
            ("input-file", po::value<string>(), "input file")
            ;

        po::options_description all_options("all options");
        all_options.add(desc).add(desc_hidden);

        po::positional_options_description positional;
        positional.add("input-file", 1);

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

        if ( vm.count( "debug" ) ) {
            logLevel = logINFO;
        }

        for ( string s = "dd"; s.length() <= 10; s.append("d") )
            if ( vm.count( s ) )
                logLevel = (TLogLevel) (logWARNING + s.length());

        if ( vm.count("input-file") )
            filename = vm["input-file"].as<string>();
    }
    catch(exception& e) {
        cerr << "Option processing error: " << e.what() << endl;
        return 1;
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";
    }

#if 0
    FILE* file = fopen("glogg.log", "w");
    Output2FILE::Stream() = file;
#endif

    FILELog::setReportingLevel( logLevel );

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

    // FIXME: should be replaced by a two staged init of MainWindow
    GetPersistentInfo().retrieve( QString( "settings" ) );

    std::unique_ptr<Session> session( new Session() );
    MainWindow mw( std::move( session ) );

    LOG(logDEBUG) << "MainWindow created.";
    mw.show();
    mw.reloadSession();
    mw.loadInitialFile( QString::fromStdString( filename ) );
    return app.exec();
}

static void print_version()
{
    cout << "glogg " GLOGG_VERSION "\n";
#ifdef GLOGG_COMMIT
    cout << "Built " GLOGG_DATE " from " GLOGG_COMMIT "\n";
#endif
    cout << "Copyright (C) 2009, 2010, 2011, 2012, 2013 Nicolas Bonnefon and other contributors\n";
    cout << "This is free software.  You may redistribute copies of it under the terms of\n";
    cout << "the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n";
    cout << "There is NO WARRANTY, to the extent permitted by law.\n";
}
