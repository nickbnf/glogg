/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov and other contributors
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

#include "tabnamemapping.h"

namespace {
constexpr const int TABNAMEMAPPING_VERSION = 1;
}

QString TabNameMapping::tabName( const QString& path ) const
{
    auto nameMapping
        = std::find_if( tabNames_.cbegin(), tabNames_.cend(),
                        [&path]( const auto& mapping ) { return mapping.path == path; } );

    if ( nameMapping != tabNames_.cend() ) {
        return nameMapping->name;
    }
    else {
        return "";
    }
}

TabNameMapping& TabNameMapping::setTabName( const QString& path, const QString& name )
{
    auto nameMapping
        = std::find_if( tabNames_.begin(), tabNames_.end(),
                        [&path]( const auto& mapping ) { return mapping.path == path; } );

    if ( nameMapping == tabNames_.end() && !name.isEmpty() ) {
        tabNames_.emplace_back( TabName{ path, name } );
    }
    else if ( nameMapping != tabNames_.end() ) {
        if ( name.isEmpty() ) {
            tabNames_.erase( nameMapping );
        }
        else {
            nameMapping->name = name;
        }
    }

    return *this;
}

void TabNameMapping::saveToStorage( QSettings& settings ) const
{
    LOG_DEBUG << "TabNameMapping::saveToStorage";

    settings.beginGroup( "TabNameMapping" );
    settings.setValue( "version", TABNAMEMAPPING_VERSION );
    settings.remove( "tabNames" );
    settings.beginWriteArray( "tabNames" );
    for ( auto i = 0u; i < tabNames_.size(); ++i ) {
        settings.setArrayIndex( static_cast<int>( i ) );
        settings.setValue( "path", tabNames_.at( i ).path );
        settings.setValue( "name", tabNames_.at( i ).name );
    }
    settings.endArray();
    settings.endGroup();
}

void TabNameMapping::retrieveFromStorage( QSettings& settings )
{
    LOG_DEBUG << "TabNameMapping::retrieveFromStorage";

    tabNames_.clear();

    if ( settings.contains( "TabNameMapping/version" ) ) {
        settings.beginGroup( "TabNameMapping" );
        if ( settings.value( "version" ).toInt() == TABNAMEMAPPING_VERSION ) {
            int size = settings.beginReadArray( "tabNames" );
            for ( auto i = 0; i < size; ++i ) {
                settings.setArrayIndex( static_cast<int>( i ) );
                QString path = settings.value( "path" ).toString();
                QString name = settings.value( "name" ).toString();

                tabNames_.emplace_back( TabName{ path, name } );
            }
            settings.endArray();
        }
        else {
            LOG_ERROR << "Unknown version of tab names mapping, ignoring it...";
        }
        settings.endGroup();
    }
}
