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

// Implements PersistentInfo, a singleton class which store/retrieve objects
// to persistent storage.

#include "persistentinfo.h"

#include <cassert>
#include <QStringList>

#include "log.h"
#include "persistable.h"

PersistentInfo::PersistentInfo()
{
    settings_    = NULL;
    initialised_ = false;
}

PersistentInfo::~PersistentInfo()
{
    if ( initialised_ )
        delete settings_;
}

void PersistentInfo::migrateAndInit()
{
    assert( initialised_ == false );
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    // On Windows, we use .ini files and import from the registry if no
    // .ini file is found (glogg <= 0.9 used the registry).

    // This store the config file in %appdata%
    settings_ = new QSettings( QSettings::IniFormat,
            QSettings::UserScope, "glogg", "glogg" );

    if ( settings_->childKeys().count() == 0 ) {
        LOG(logWARNING) << "INI file empty, trying to import from registry";
        QSettings registry( "glogg", "glogg" );
        foreach ( QString key, registry.allKeys() ) {
            settings_->setValue( key, registry.value( key ) );
        }
    }
#else
    // We use default Qt storage on proper OSes
    settings_ = new QSettings( "glogg", "glogg" );
#endif
    initialised_ = true;
}

void PersistentInfo::registerPersistable( std::shared_ptr<Persistable> object,
        const QString& name )
{
    assert( initialised_ );

    objectList_.insert( name, object );
}

std::shared_ptr<Persistable> PersistentInfo::getPersistable( const QString& name )
{
    assert( initialised_ );

    std::shared_ptr<Persistable> object = objectList_.value( name, NULL );

    return object;
}

void PersistentInfo::save( const QString& name )
{
    assert( initialised_ );

    auto object = objectList_.value( name );
    if ( object )
        save( *object );
    else
        LOG(logERROR) << "Unregistered persistable " << name.toStdString();
}

void PersistentInfo::save( const Persistable& persistable )
{
    assert( initialised_ );

    persistable.saveToStorage( *settings_ );

    // Sync to ensure it is propagated to other processes
    settings_->sync();
}

void PersistentInfo::retrieve( const QString& name )
{
    assert( initialised_ );

    auto object = objectList_.value( name );
    if ( object )
        retrieve( *object );
    else
        LOG(logERROR) << "Unregistered persistable " << name.toStdString();
}

void PersistentInfo::retrieve( Persistable& persistable )
{
    assert( initialised_ );

    // Sync to ensure it has been propagated from other processes
    settings_->sync();

    persistable.retrieveFromStorage( *settings_ );
}

// Friend function to construct/get the singleton
PersistentInfo& GetPersistentInfo()
{
    static PersistentInfo pInfo;
    return pInfo;
}

