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

#ifndef KLOGG_PERSISTABLE_H
#define KLOGG_PERSISTABLE_H

#include <type_traits>

#include "log.h"
#include "persistentinfo.h"

class QSettings;

template <typename T, typename SettingsType = app_settings>
class Persistable {

  public:
    static T& get()
    {
        return getPersistable( false );
    }

    static T& getSynced()
    {
        auto& persistable = getPersistable( true );
        persistable.retrieve();
        return persistable;
    }

    void save() const
    {
        auto& settings = PersistentInfo::getSettings( SettingsType{} );
        static_cast<const T&>( *this ).saveToStorage( settings );
    }

  private:
    static T& getPersistable( bool willBeInitialized = false )
    {
        static bool persistableInitialized = false;
        if ( !persistableInitialized && !willBeInitialized ) {
            LOG_ERROR << "Access to not initialized persistable " << T::persistableName();
            throw std::logic_error( "Access to not initialized persistable" );
        }

        if ( !persistableInitialized ) {
            persistableInitialized = willBeInitialized;
        }

        static T persistable;
        return persistable;
    }

    void retrieve()
    {
        auto& settings = PersistentInfo::getSettings( SettingsType{} );

        settings.sync();
        static_cast<T&>( *this ).retrieveFromStorage( settings );
    }
};

#endif
