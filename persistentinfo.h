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

#ifndef PERSISTENTINFO_H
#define PERSISTENTINFO_H

#include <QSettings>
#include <QHash>

class Persistable;

// Singleton class managing the saving of persistent data to permanent storage
// Clients must implement Persistable and register with this object, they can
// then be saved/loaded.
class PersistentInfo {
  public:
    // Initialise the storage backend for the Persistable, migrating the settings
    // if needed. Must be called before any other function.
    void migrateAndInit();
    // Register a Persistable
    void registerPersistable( Persistable* object, const QString& name );
    // Get a Persistable (or NULL if it doesn't exist)
    Persistable* getPersistable( const QString& name );
    // Save a persistable to its permanent storage
    void save( const QString& name );
    // Retrieve a persistable from permanent storage
    void retrieve( const QString& name );

  private:
    // Can't be constructed or copied (singleton)
    PersistentInfo();
    PersistentInfo( const PersistentInfo& );
    ~PersistentInfo();

    // Has migrateAndInit() been called?
    bool initialised_;

    // List of persistables
    QHash<QString, Persistable*> objectList_;

    // Qt setting object
    QSettings* settings_;

    // allow this function to create one instance
    friend PersistentInfo& GetPersistentInfo();
};

PersistentInfo& GetPersistentInfo();

// Global function used to get an object from the PersistentInfo store
template<typename T>
T& Persistent( const char* name )
{
    Persistable* p = GetPersistentInfo().getPersistable( QString( name ) );
    return dynamic_cast<T&>(*p);
}

#endif
