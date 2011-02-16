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

// Implements PersistentInfo, a class which store/retrieve objects
// to persistent storage.

#include "persistentinfo.h"

#include "log.h"
#include "persistable.h"

PersistentInfo::PersistentInfo() : settings_( "glogg", "glogg" )
{
}

void PersistentInfo::registerPersistable( Persistable* object,
        const QString& name )
{
    objectList_.insert( name, object );
}

Persistable* PersistentInfo::getPersistable( const QString& name )
{
    Persistable* object = objectList_.value( name, NULL );

    return object;
}

void PersistentInfo::save( const QString& name )
{
    if ( objectList_.contains( name ) )
        objectList_.value( name )->saveToStorage( settings_ );
    else
        LOG(logERROR) << "Unregistered persistable " << name.toStdString();

    // Sync to ensure it is propagated to other processes
    settings_.sync();
}

void PersistentInfo::retrieve( const QString& name )
{
    // Sync to ensure it has been propagated from other processes
    settings_.sync();

    if ( objectList_.contains( name ) )
        objectList_.value( name )->retrieveFromStorage( settings_ );
    else
        LOG(logERROR) << "Unregistered persistable " << name.toStdString();
}

// Friend function to construct/get the singleton
PersistentInfo& GetPersistentInfo()
{
    static PersistentInfo pInfo;
    return pInfo;
}

