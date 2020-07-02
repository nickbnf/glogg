/*
 * Copyright (C) 2009, 2010, 2011 Nicolas Bonnefon and other contributors
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

// This file implements class SavedSearch

#include <QDataStream>
#include <QSettings>

#include "log.h"
#include "savedsearches.h"

SavedSearches::SavedSearches()
    : savedSearches_()
{
    qRegisterMetaTypeStreamOperators<SavedSearches>( "SavedSearches" );
}

void SavedSearches::addRecent( const QString& text )
{
    // We're not interested in blank lines
    if ( text.isEmpty() )
        return;

    // Remove any copy of the about to be added text
    savedSearches_.removeAll( text );

    // Add at the front
    savedSearches_.push_front( text );

    // Trim the list if it's too long
    while ( savedSearches_.size() > maxNumberOfRecentSearches )
        savedSearches_.pop_back();
}

QStringList SavedSearches::recentSearches() const
{
    return savedSearches_;
}

void SavedSearches::clear()
{
    savedSearches_.clear();
}

//
// Operators for serialization
//

QDataStream& operator<<( QDataStream& out, const SavedSearches& object )
{
    LOG( logDEBUG ) << "<<operator from SavedSearches";

    out << object.savedSearches_;

    return out;
}

QDataStream& operator>>( QDataStream& in, SavedSearches& object )
{
    LOG( logDEBUG ) << ">>operator from SavedSearches";

    in >> object.savedSearches_;

    return in;
}

//
// Persistable virtual functions implementation
//

void SavedSearches::saveToStorage( QSettings& settings ) const
{
    LOG( logDEBUG ) << "SavedSearches::saveToStorage";

    settings.beginGroup( "SavedSearches" );
    settings.setValue( "version", SAVEDSEARCHES_VERSION );
    settings.remove( "searchHistory" );
    settings.beginWriteArray( "searchHistory" );
    for ( int i = 0; i < savedSearches_.size(); ++i ) {
        settings.setArrayIndex( i );
        settings.setValue( "string", savedSearches_.at( i ) );
    }
    settings.endArray();
    settings.endGroup();
}

void SavedSearches::retrieveFromStorage( QSettings& settings )
{
    LOG( logDEBUG ) << "SavedSearches::retrieveFromStorage";

    savedSearches_.clear();

    if ( settings.contains( "SavedSearches/version" ) ) {
        settings.beginGroup( "SavedSearches" );
        if ( settings.value( "version" ).toInt() == SAVEDSEARCHES_VERSION ) {
            int size = settings.beginReadArray( "searchHistory" );
            for ( int i = 0; i < size; ++i ) {
                settings.setArrayIndex( i );
                QString search = settings.value( "string" ).toString();
                savedSearches_.append( search );
            }
            settings.endArray();
        }
        else {
            LOG( logERROR ) << "Unknown version of saved searches, ignoring it...";
        }
        settings.endGroup();
    }
    else {
        LOG( logWARNING ) << "Trying to import legacy (<=0.8.2) saved searches...";
        SavedSearches tmp_saved_searches = settings.value( "savedSearches" ).value<SavedSearches>();
        *this = tmp_saved_searches;
        LOG( logWARNING ) << "...imported searches: " << savedSearches_.count() << " elements";
        // Remove the old key once migration is done
        settings.remove( "savedSearches" );
        // And replace it with the new one
        saveToStorage( settings );
        settings.sync();
    }
}
