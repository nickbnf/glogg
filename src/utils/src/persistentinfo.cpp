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
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include "log.h"
#include "persistable.h"

PersistentInfo::PersistentInfo()
{
    settings_    = nullptr;
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

    QString configurationFile = QDir::cleanPath( qApp->applicationDirPath() +
                                       QDir::separator() + "klogg.conf" );

    if ( QFileInfo::exists(configurationFile) ) {
        settings_ = new QSettings(configurationFile, QSettings::IniFormat);
    }
    else {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        settings_ = new QSettings( QSettings::IniFormat,
                QSettings::UserScope, "klogg", "klogg" );
#else
        // We use default Qt storage on proper OSes
        settings_ = new QSettings( "klogg", "klogg" );
#endif
    }
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

    std::shared_ptr<Persistable> object = objectList_.value( name, nullptr );

    return object;
}

void PersistentInfo::save( const QString& name )
{
    assert( initialised_ );

    if ( objectList_.contains( name ) )
        objectList_.value( name )->saveToStorage( *settings_ );
    else
        LOG(logERROR) << "Unregistered persistable " << name.toStdString();

    // Sync to ensure it is propagated to other processes
    settings_->sync();
}

void PersistentInfo::retrieve( const QString& name )
{
    assert( initialised_ );

    // Sync to ensure it has been propagated from other processes
    settings_->sync();

    if ( objectList_.contains( name ) )
        objectList_.value( name )->retrieveFromStorage( *settings_ );
    else
        LOG(logERROR) << "Unregistered persistable " << name.toStdString();
}

// Friend function to construct/get the singleton
PersistentInfo& GetPersistentInfo()
{
    static PersistentInfo pInfo;
    return pInfo;
}

