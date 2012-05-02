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

// This file implements class RecentEncodings

#include <QSettings>
#include <QTextCodec>

#include "log.h"
#include "recentencodings.h"

const int RecentEncodings::RECENTENCODINGS_VERSION = 1;
const int RecentEncodings::MAX_NUMBER_OF_ENCODINGS = 5;

RecentEncodings::RecentEncodings() : recentEncodings_()
{
}

int RecentEncodings::max()
{
    return MAX_NUMBER_OF_ENCODINGS;
}

void RecentEncodings::addRecent( const QString& encoding )
{
    // Invariant: no duplicates
    recentEncodings_.removeAll( encoding );

    // Add at the front
    recentEncodings_.push_front( encoding );

    // Trim the list if it's too long
    while ( recentEncodings_.size() > MAX_NUMBER_OF_ENCODINGS )
        recentEncodings_.pop_back();
}

QStringList RecentEncodings::recentEncodings() const
{
    return recentEncodings_;
}

//
// Persistable virtual functions implementation
//

void RecentEncodings::saveToStorage( QSettings& settings ) const
{
    LOG(logDEBUG) << "RecentEncodings::saveToStorage";

    settings.beginGroup( "RecentEncodings" );
    settings.setValue( "version", RECENTENCODINGS_VERSION );
    settings.beginWriteArray( "encodingsHistory" );
    for (int i = 0; i < recentEncodings_.size(); ++i) {
        settings.setArrayIndex( i );
        settings.setValue( "name", recentEncodings_.at( i ) );
    }
    settings.endArray();
    settings.endGroup();
}

void RecentEncodings::retrieveFromStorage( QSettings& settings )
{
    LOG(logDEBUG) << "RecentEncodings::retrieveFromStorage";

    recentEncodings_.clear();

    if ( settings.contains( "RecentEncodings/version" ) ) {
        // Unserialise the "new style" stored history
        settings.beginGroup( "RecentEncodings" );
        if ( settings.value( "version" ) == RECENTENCODINGS_VERSION ) {
            int size = settings.beginReadArray( "encodingsHistory" );
            if ( size > MAX_NUMBER_OF_ENCODINGS ) {
                LOG(logINFO) << "More persisted encodings (" << size << ") "
                    << "than maximum (" << MAX_NUMBER_OF_ENCODINGS << "): "
                    << "skipped the extra";
            }
            const int count = qMin( size, MAX_NUMBER_OF_ENCODINGS );
            for (int i = 0; i < count; ++i) {
                settings.setArrayIndex(i);
                QString encoding = settings.value( "name" ).toString();
                // Don't trust persisted data: might come from another system
                // NOTE: This introduces a delay on startup, since this causes
                // the codecs (for these recently used encodings) to actually
                // be loaded. This is not strictly necessary, however handling
                // a possibly invalid codec that's already in the menu does
                // not sound appealing -- so validating here for now.
                if ( QTextCodec::codecForName( encoding.toUtf8() ) )
                    recentEncodings_.append( encoding );
                else
                    LOG(logINFO) << "Unsupported persisted encoding '"
                        << encoding.toStdString() << "': skipped";
            }
            settings.endArray();
        }
        else {
            LOG(logERROR)
                << "Unknown version of RecentEncodings, ignoring it...";
        }
        settings.endGroup();
    }
}
