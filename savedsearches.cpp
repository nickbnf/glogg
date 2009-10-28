/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
 *
 * This file is part of LogCrawler.
 *
 * LogCrawler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LogCrawler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LogCrawler.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "log.h"

#include "savedsearches.h"

SavedSearches::SavedSearches() : savedSearches_()
{
}

void SavedSearches::addRecent( const QString& text )
{
    // Remove any copy of the about to be added text
    savedSearches_.removeAll( text );

    // Add at the front
    savedSearches_.push_front( text );

    // Trim the list if it's too long
    while (savedSearches_.size() > maxNumberOfRecentSearches)
        savedSearches_.pop_back();
}

QStringList SavedSearches::recentSearches() const
{
    return savedSearches_;
}

//
// Operators for serialization
//

QDataStream& operator<<( QDataStream& out, const SavedSearches& object )
{
    out << object.savedSearches_;

    return out;
}

QDataStream& operator>>( QDataStream& in, SavedSearches& object )
{
    in >> object.savedSearches_;

    return in;
}
