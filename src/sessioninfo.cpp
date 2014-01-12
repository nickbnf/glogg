/*
 * Copyright (C) 2011, 2014 Nicolas Bonnefon and other contributors
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

#include "sessioninfo.h"

#include <QSettings>

#include "log.h"

const int SessionInfo::OPENFILES_VERSION = 1;

void SessionInfo::retrieveFromStorage( QSettings& settings )
{
    LOG(logDEBUG) << "SessionInfo::retrieveFromStorage";

    /*
    geometry_     = settings.value("geometry").toByteArray();
    crawlerState_ = settings.value("crawlerWidget").toByteArray();
    */
    if ( settings.contains( "OpenFiles/version" ) ) {
        // Unserialise the "new style" stored history
        settings.beginGroup( "OpenFiles" );
        if ( settings.value( "version" ) == OPENFILES_VERSION ) {
            int size = settings.beginReadArray( "openFiles" );
            for (int i = 0; i < size; ++i) {
                settings.setArrayIndex(i);
                std::string file_name =
                    settings.value( "fileName" ).toString().toStdString();
                uint64_t top_line = settings.value( "topLine" ).toInt();
                openFiles_.push_back( { file_name, top_line } );
            }
            settings.endArray();
        }
        else {
            LOG(logERROR) << "Unknown version of OpenFiles, ignoring it...";
        }
        settings.endGroup();
    }
}

void SessionInfo::saveToStorage( QSettings& settings ) const
{
    LOG(logDEBUG) << "SessionInfo::saveToStorage";

    /*
    settings.setValue( "geometry", geometry_ );
    settings.setValue( "crawlerWidget", crawlerState_ );
    */
    settings.beginGroup( "OpenFiles" );
    settings.setValue( "version", OPENFILES_VERSION );
    settings.beginWriteArray( "openFiles" );
    for ( int i = 0; i < openFiles_.size(); ++i ) {
        settings.setArrayIndex( i );
        const OpenFile* open_file = &(openFiles_.at( i ));
        settings.setValue( "fileName", QString( open_file->fileName.c_str() ) );
        settings.setValue( "topLine", qint64( open_file->topLine ) );
    }
    settings.endArray();
    settings.endGroup();
}
