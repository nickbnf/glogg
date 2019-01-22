/*
 * Copyright (C) 2010, 2014, 2015 Nicolas Bonnefon and other contributors
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

#include "platformfilewatcher.h"

#include "log.h"

std::shared_ptr<PlatformFileWatcher::PlatformWatchTower> PlatformFileWatcher::watch_tower_;

PlatformFileWatcher::PlatformFileWatcher() : FileWatcher()
{
    // Caution, this is NOT thread-safe or re-entrant!
    if ( !watch_tower_ )
    {
        watch_tower_ = std::make_shared<PlatformWatchTower>();
    }
}

PlatformFileWatcher::~PlatformFileWatcher()
{
}

void PlatformFileWatcher::addFile( const QString& fileName )
{
    LOG(logDEBUG) << "FileWatcher::addFile " << fileName.toStdString();

    watched_file_name_ = fileName;

    removeFile( fileName );
    notification_ = std::make_shared<Registration>(
            watch_tower_->addFile( fileName.toStdString(), [this, fileName] {
                emit fileChanged( fileName ); } ) );
}

void PlatformFileWatcher::removeFile( const QString& fileName )
{
    LOG(logDEBUG) << "FileWatcher::removeFile " << fileName.toStdString();

    notification_ = nullptr;
}

void PlatformFileWatcher::setPollingInterval( uint32_t interval_ms )
{
    watch_tower_->setPollingInterval( interval_ms );
}
