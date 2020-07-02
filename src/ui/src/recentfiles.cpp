/*
 * Copyright (C) 2011 Nicolas Bonnefon and other contributors
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

#include <QSettings>
#include <QFile>

#include "log.h"
#include "recentfiles.h"

void RecentFiles::addRecent( const QString& text )
{
    // First prune non existent files
    QMutableStringListIterator i(recentFiles_);
    while ( i.hasNext() ) {
        if ( !QFile::exists(i.next()) )
            i.remove();
    }

    // Remove any copy of the about to be added filename
    recentFiles_.removeAll( text );

    // Add at the front
    recentFiles_.push_front( text );

    // Trim the list if it's too long
    while ( recentFiles_.size() > MAX_NUMBER_OF_FILES )
        recentFiles_.pop_back();
}

QStringList RecentFiles::recentFiles() const
{
    return recentFiles_;
}

//
// Persistable virtual functions implementation
//

void RecentFiles::saveToStorage( QSettings& settings ) const
{
    LOG(logDEBUG) << "RecentFiles::saveToStorage";

    settings.beginGroup( "RecentFiles" );
    settings.setValue( "version", RECENTFILES_VERSION );
    settings.remove( "filesHistory" );
    settings.beginWriteArray( "filesHistory" );
    for (int i = 0; i < recentFiles_.size(); ++i) {
        settings.setArrayIndex( i );
        settings.setValue( "name", recentFiles_.at( i ) );
    }
    settings.endArray();
    settings.endGroup();
}

void RecentFiles::retrieveFromStorage( QSettings& settings )
{
    LOG(logDEBUG) << "RecentFiles::retrieveFromStorage";

    recentFiles_.clear();

    if ( settings.contains( "RecentFiles/version" ) ) {
        settings.beginGroup( "RecentFiles" );
        if ( settings.value( "version" ).toInt() == RECENTFILES_VERSION ) {
            int size = settings.beginReadArray( "filesHistory" );
            for (int i = 0; i < size; ++i) {
                settings.setArrayIndex(i);
                QString file = settings.value( "name" ).toString();
                recentFiles_.append( file );
            }
            settings.endArray();
        }
        else {
            LOG(logERROR) << "Unknown version of recent files, ignoring it...";
        }
        settings.endGroup();
    }
}
