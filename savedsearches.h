/*
 * Copyright (C) 2009, 2011 Nicolas Bonnefon and other contributors
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

#ifndef SAVEDSEARCHES_H
#define SAVEDSEARCHES_H

#include <QString>
#include <QStringList>
#include <QMetaType>

#include "persistable.h"

// Keeps track of the previously used searches and allows the application
// to retrieve them.
class SavedSearches : public Persistable
{
  public:
    // Creates an empty set of saved searches
    SavedSearches();

    // Adds the passed search to the list of recently used searches
    void addRecent( const QString& text );

    // Returns a list of recent searches (newer first)
    QStringList recentSearches() const;

    // Operators for serialization
    // (only for migrating pre 0.8.2 settings, will be removed)
    friend QDataStream& operator<<( QDataStream& out, const SavedSearches& object );
    friend QDataStream& operator>>( QDataStream& in, SavedSearches& object );

    // Reads/writes the current config in the QSettings object passed
    void saveToStorage( QSettings& settings ) const;
    void retrieveFromStorage( QSettings& settings );

  private:
    static const int SAVEDSEARCHES_VERSION;
    static const int maxNumberOfRecentSearches;

    QStringList savedSearches_;
};

Q_DECLARE_METATYPE(SavedSearches)

#endif
