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

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <cassert>

#include "log.h"
#include "persistable.h"

void PersistentInfo::migrateAndInit( SettingsStorage storage )
{
    assert( !settings_ );

    QString configurationFile
        = QDir::cleanPath( qApp->applicationDirPath() + QDir::separator() + "klogg.conf" );

    if ( storage == Portable || QFileInfo::exists( configurationFile ) ) {
        settings_ = std::make_unique<QSettings>( configurationFile, QSettings::IniFormat );
    }
    else {
#ifdef Q_OS_WIN
        settings_ = std::make_unique<QSettings>( QSettings::IniFormat, QSettings::UserScope,
                                                 "klogg", "klogg" );
#else
        // We use default Qt storage on proper OSes
        settings_ = std::make_unique<QSettings>( "klogg", "klogg" );
#endif
    }
}

void PersistentInfo::registerPersistable( std::unique_ptr<Persistable> object, const char* name )
{
    assert( settings_ );

    objectList_.emplace( name, std::move(object) );
}

Persistable& PersistentInfo::getPersistable( const char* name ) const
{
    assert( settings_ );

    auto& object = objectList_[name];

    return *object.get();
}

void PersistentInfo::save( const char* name ) const
{
    assert( settings_ );

    if ( objectList_.count( name ) )
        objectList_[name]->saveToStorage( *settings_ );
    else
        LOG( logERROR ) << "Unregistered persistable " << name;

    // Sync to ensure it is propagated to other processes
    settings_->sync();
}

void PersistentInfo::retrieve( const char* name ) const
{
    assert( settings_ );

    // Sync to ensure it has been propagated from other processes
    settings_->sync();

    if ( objectList_.count( name ) )
        objectList_[name]->retrieveFromStorage( *settings_ );
    else
        LOG( logERROR ) << "Unregistered persistable " << name;
}

// Friend function to construct/get the singleton
PersistentInfo& GetPersistentInfo()
{
    static PersistentInfo pInfo;
    return pInfo;
}
