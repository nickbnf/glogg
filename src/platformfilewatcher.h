/*
 * Copyright (C) 2014, 2015 Nicolas Bonnefon and other contributors
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

#ifndef PLATFORMFILEWATCHER_H
#define PLATFORMFILEWATCHER_H

#include "filewatcher.h"

#include <memory>


#ifdef _WIN32
#  include "winwatchtowerdriver.h"
#elif defined(__APPLE__)
#  include "kqueuewatchtowerdriver.h"
#else
#  include "inotifywatchtowerdriver.h"
#endif

#include "watchtower.h"

class INotifyWatchTower;

// Please note that due to the implementation of the constructor
// this class is not thread safe and shall always be used from the main UI thread.
class PlatformFileWatcher : public FileWatcher {
  Q_OBJECT

  public:
    // Create the empty object
    PlatformFileWatcher();
    // Destroy the object
    ~PlatformFileWatcher();

    void addFile( const QString& fileName );
    void removeFile( const QString& fileName );

    // Set the polling interval (0 means disabled)
    void setPollingInterval( uint32_t interval_ms );

  signals:
    void fileChanged( const QString& );

  private:
#ifdef _WIN32
#  ifdef HAS_TEMPLATE_ALIASES
using PlatformWatchTower = WatchTower<WinWatchTowerDriver>;
#  else
typedef WatchTower<WinWatchTowerDriver> PlatformWatchTower;
#  endif
#elif defined(__APPLE__)
using PlatformWatchTower = WatchTower<KQueueWatchTowerDriver>;
#else
#  ifdef HAS_TEMPLATE_ALIASES
using PlatformWatchTower = WatchTower<INotifyWatchTowerDriver>;
#  else
typedef WatchTower<INotifyWatchTowerDriver> PlatformWatchTower;
#  endif
#endif

    // The following variables are protected by watched_files_mutex_
    QString watched_file_name_;

    // Reference to the (unique) watchtower.
    static std::shared_ptr<PlatformWatchTower> watch_tower_;

    std::shared_ptr<Registration> notification_;
};

#endif
