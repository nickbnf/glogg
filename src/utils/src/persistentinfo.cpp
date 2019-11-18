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

// Implements PersistentInfo, a singleton class which store/retrieve objects
// to persistent storage.

#include "persistentinfo.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include "log.h"
#include "persistable.h"

constexpr uint8_t AppSettingsVersion = 1;
constexpr uint8_t SessionSettingsVersion = 1;

PersistentInfo::ConfigFileParameters::ConfigFileParameters()
{
    QString portableConfigPath = qApp->applicationDirPath() + QDir::separator() + "klogg.conf";
    if ( forcePortable || QFileInfo::exists( portableConfigPath ) ) {
        format = QSettings::IniFormat;
        appSettingsPath = portableConfigPath;
    }
    else {
#ifdef Q_OS_WIN
        format = QSettings::IniFormat;
#endif
        appSettingsPath = QSettings{ format, QSettings::UserScope, "klogg", "klogg" }.fileName();
    }

    sessionSettingsPath
        = QFileInfo( appSettingsPath ).absoluteDir().filePath( "klogg_session.conf" );
    if ( !QFileInfo::exists( sessionSettingsPath ) && QFileInfo::exists( appSettingsPath ) ) {
        QFile::copy( appSettingsPath, sessionSettingsPath );
    }
}

PersistentInfo::PersistentInfo( const ConfigFileParameters& config )
    : appSettings_{ config.appSettingsPath, config.format }
    , sessionSettings_{ config.sessionSettingsPath, config.format }
{
    const auto oldAppSettingsVerson = appSettings_.value( "version", 0 ).toUInt();
    if ( oldAppSettingsVerson != AppSettingsVersion ) {
        appSettings_.remove( "geometry" );
        appSettings_.remove( "versionchecker.nextDeadline" );
        appSettings_.remove( "OpenFiles" );
        appSettings_.remove( "RecentFiles" );
        appSettings_.remove( "SavedSearches" );
    }

    const auto oldSessionSettinsVersion = sessionSettings_.value( "version", 0 ).toUInt();
    if ( oldSessionSettinsVersion != SessionSettingsVersion ) {
        sessionSettings_.setValue( "Window/geometry", sessionSettings_.value( "geometry" ) );
        sessionSettings_.setValue( "VersionChecker/nextDeadline",
                                   sessionSettings_.value( "versionchecker.nextDeadline" ) );
        sessionSettings_.remove( "HighlighterSet" );
        for ( const auto& key : sessionSettings_.childKeys() ) {
            sessionSettings_.remove( key );
        }
    }

    appSettings_.setValue( "version", AppSettingsVersion );
    sessionSettings_.setValue( "version", SessionSettingsVersion );
}

PersistentInfo& PersistentInfo::getInstance()
{
    static PersistentInfo pInfo;
    return pInfo;
}

QSettings& PersistentInfo::getSettings( app_settings )
{
    return getInstance().appSettings_;
}

QSettings& PersistentInfo::getSettings( session_settings )
{
    return getInstance().sessionSettings_;
}
