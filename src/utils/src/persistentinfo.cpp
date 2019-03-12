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


PersistentInfo::ConfigFileParameters::ConfigFileParameters()
{
    QString portableConfigPath = qApp->applicationDirPath() + QDir::separator() + "klogg.conf";
    if ( forcePortable || QFileInfo::exists( portableConfigPath ) ) {
        format = QSettings::IniFormat;
        path = portableConfigPath;
    }
    else {
#ifdef Q_OS_WIN
        format = QSettings::IniFormat;
#endif
        path = QSettings{ format, QSettings::UserScope, "klogg", "klogg" }.fileName();
    }
}

PersistentInfo::PersistentInfo( ConfigFileParameters config )
    : settings_{ config.path, config.format }
{
}

QSettings& PersistentInfo::getSettings()
{
    static PersistentInfo pInfo;
    return pInfo.settings_;
}

void Persistable::save() const
{
    auto& settings = PersistentInfo::getSettings();
    saveToStorage( settings );
    settings.sync();
}

void Persistable::retrieve()
{
    auto& settings = PersistentInfo::getSettings();
    settings.sync();
    retrieveFromStorage( settings );
}
