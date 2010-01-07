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

#include "mainwindow.h"
#include "log.h"

int main(int argc, char *argv[])
{
    const char* filename = "";

#if 0
    FILE* file = fopen("glogg.log", "w");
    Output2FILE::Stream() = file;
#endif

    QApplication app(argc, argv);

    if ( argc > 0 ) {
        /* Filename provided as an argument */
        filename = argv[1];
    }

    MainWindow* mw = new MainWindow();

    LOG(logDEBUG) << "MainWindow created.";
    mw->show();
    mw->loadInitialFile( QString(filename) );
    return app.exec();
}
