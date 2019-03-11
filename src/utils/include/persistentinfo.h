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

/*
 * Copyright (C) 2019 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PERSISTENTINFO_H
#define PERSISTENTINFO_H

#include <memory>

#include <unordered_map>

#include <QSettings>

class Persistable;

// Singleton class managing the saving of persistent data to permanent storage
// Clients must implement Persistable and register with this object, they can
// then be saved/loaded.
class PersistentInfo {
  public:
    // Register a Persistable
    void registerPersistable( std::unique_ptr<Persistable> object, const char* name );
    // Get a Persistable (or NULL if it doesn't exist)
    Persistable& getPersistable( const char* name );
    // Save a persistable to its permanent storage
    void save( const char* name );
    // Retrieve a persistable from permanent storage
    void retrieve( const char* name );

  private:
    struct ConfigFileParameters {
        QString path;
        QSettings::Format format;

        ConfigFileParameters();

        static const bool forcePortable;
    };

    // Can't be constructed or copied (singleton)
    PersistentInfo( ConfigFileParameters config = {} );

    PersistentInfo( const PersistentInfo& ) = delete;
    PersistentInfo& operator=( const PersistentInfo& ) = delete;
    PersistentInfo( PersistentInfo&& ) = delete;
    PersistentInfo& operator=( PersistentInfo&& ) = delete;

    // List of persistables
    std::unordered_map<const char*, std::unique_ptr<Persistable>> objectList_;

    // Qt setting object
    QSettings settings_;

    // allow this function to create one instance
    friend PersistentInfo& GetPersistentInfo();
};

PersistentInfo& GetPersistentInfo();

// Global function used to get a reference to an object
// from the PersistentInfo store
template <typename T> T& Persistent( const char* name )
{
    auto& p = GetPersistentInfo().getPersistable( name );
    return static_cast<T&>( p );
}
#endif
