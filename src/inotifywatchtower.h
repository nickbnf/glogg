/*
 * Copyright (C) 2014 Nicolas Bonnefon and other contributors
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

#ifndef INOTIFYWATCHTOWER_H
#define INOTIFYWATCHTOWER_H

#include "watchtower.h"

#include <thread>
#include <atomic>
#include <mutex>

// The WatchTower keeps track of a set of files and asynchronously
// signal changes to them.
// In glogg, there is only one instance of the WatchTower and it
// keeps track of all the open files.
// This is implemented using Linux specific inotify.
class INotifyWatchTower : public WatchTower {
  public:
    // Create an empty watchtower
    INotifyWatchTower();
    // Destroy the object
    ~INotifyWatchTower();

    Registration addFile( const std::string& file_name,
            std::function<void()> notification ) override;

  private:
    ObservedFileList observed_file_list_;

    // Protects the observed_file_list_
    std::mutex observers_mutex_;

    // Thread
    std::atomic_bool running_;
    std::thread thread_;

    // Only written at initialisation so no protection needed.
    const int inotify_fd;

    // Exist as long as the onject exists, to ensure observers won't try to
    // call us if we are dead.
    std::shared_ptr<void> heartBeat_;

    // Private member functions
    static void removeNotification( INotifyWatchTower* watch_tower,
            std::shared_ptr<void> notification );
    void run();
};

#endif
