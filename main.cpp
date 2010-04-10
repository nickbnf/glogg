/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
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

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
using namespace std;

#include "mainwindow.h"
#include "log.h"

static void print_version();

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    string filename = "";

    try {
        po::options_description desc("Usage: glogg [options] [file]");
        desc.add_options()
            ("help,h", "print out program usage (this message)")
            ("version,v", "print glogg's version information")
            ;
        po::options_description desc_hidden("Hidden options");
        desc_hidden.add_options()
            ("input-file", po::value<string>(), "input file")
            ;

        po::positional_options_description positional;
        positional.add("input-file", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                options(desc.add(desc_hidden)).positional(positional).run(),
                vm);
        po::notify(vm);

        if ( vm.count("help") ) {
            cout << desc << "\n";
            return 1;
        }

        if ( vm.count("version") ) {
            print_version();
            return 1;
        }

        if ( vm.count("input-file") )
            filename = vm["input-file"].as<string>();
    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";
    }

#if 0
    FILE* file = fopen("glogg.log", "w");
    Output2FILE::Stream() = file;
#endif

    MainWindow* mw = new MainWindow();

    LOG(logDEBUG) << "MainWindow created.";
    mw->show();
    mw->loadInitialFile( QString::fromStdString( filename ) );
    return app.exec();
}

static void print_version()
{
    cout << "glogg " GLOGG_VERSION "\n";
#ifdef GLOGG_COMMIT
    cout << "Built " GLOGG_DATE " from " GLOGG_COMMIT "\n";
#endif
    cout << "Copyright (C) 2009 Nicolas Bonnefon and other contributors\n";
    cout << "This is free software.  You may redistribute copies of it under the terms of\n";
    cout << "the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n";
    cout << "There is NO WARRANTY, to the extent permitted by law.\n";
}
