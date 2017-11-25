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

#ifndef WINFILEWATCHER_H
#define WINFILEWATCHER_H

#include "filewatcher.h"

#include <memory>

// #include "inotifywatchtower.h"

// An implementation of FileWatcher, as an adapter to INotifyWatchTower.
// This is Linux only, and require a recent version of the kernel.

// Please note that due to the implementation of the constructor
// this class is not thread safe and shall always be used from the main UI thread.
class WinFileWatcher : public FileWatcher {
  Q_OBJECT

  public:
    // Create the empty object
    WinFileWatcher();
    // Destroy the object
    ~WinFileWatcher();

    void addFile( const QString& fileName ) override;
    void removeFile( const QString& fileName ) override;

  signals:
    void fileChanged( const QString& );

  private:
    // The following variables are protected by watched_files_mutex_
    QString watched_file_name_;

    // Reference to the (unique) watchtower.
    // static std::shared_ptr<INotifyWatchTower> watch_tower_;

    // std::shared_ptr<INotifyWatchTower::Registration> notification_;
};

#endif
