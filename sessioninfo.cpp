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

#include "sessioninfo.h"

#include <QSettings>

#include "log.h"

void SessionInfo::retrieveFromStorage( QSettings& settings )
{
    LOG(logDEBUG) << "SessionInfo::retrieveFromStorage";

    geometry_     = settings.value("geometry").toByteArray();
    crawlerState_ = settings.value("crawlerWidget").toByteArray();
    currentFile_  = settings.value("currentFile").toString();
}

void SessionInfo::saveToStorage( QSettings& settings ) const
{
    LOG(logDEBUG) << "SessionInfo::saveToStorage";

    settings.setValue( "geometry", geometry_ );
    settings.setValue( "crawlerWidget", crawlerState_ );
    settings.setValue( "currentFile", currentFile_ );
}
