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

#include <vector>
#include <list>
#include <functional>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>


// The WatchTower keeps track of a set of files and asynchronously
// signal changes to them.
// In glogg, there is only one instance of the WatchTower and it
// keeps track of all the open files.
// This is implemented using Linux specific inotify.
class INotifyWatchTower {
  public:
    // Registration object to implement RAII
    using Registration = std::shared_ptr<void>;

    // Create an empty watchtower
    INotifyWatchTower();
    // Destroy the object
    ~INotifyWatchTower();

    // Add a file to the notification list. notification will be called when
    // the file is modified, moved or deleted.
    // Lifetime of the notification is tied to the Registration object returned.
    // Note the notification function is called with a mutex held and in a
    // third party thread, beware of races!
    Registration addFile( const std::string& file_name,
            std::function<void()> notification );

  private:
    // List of files and observers
    struct ObservedDir {
        std::string path;
        int dir_wd_;
    };

    struct ObservedFile {
        ObservedFile(
                const std::string file_name,
                std::shared_ptr<void> callback,
                int file_wd,
                int symlink_wd ) : file_name_( file_name ) {
            callbacks.push_back( callback );

            file_wd_    = file_wd;
            symlink_wd_ = symlink_wd;
            dir_        = nullptr;
        }

        void addCallback( std::shared_ptr<void> callback ) {
            callbacks.push_back( callback );
        }

        std::string file_name_;
        // List of callbacks for this file
        std::vector<std::shared_ptr<void>> callbacks;

        // watch descriptor for the file itself
        int file_wd_;
        // watch descriptor for the symlink (if file is a symlink)
        int symlink_wd_;

        // link to the dir containing the file
        std::shared_ptr<ObservedDir> dir_;
    };

    class ObservedFileList {
      public:
        ObservedFileList() = default;
        ~ObservedFileList() = default;

        ObservedFile* searchByName( const std::string& file_name );
        ObservedFile* searchByFileOrSymlinkWd( int wd );
        ObservedFile* searchByDirWdAndName( int wd, const char* name );

        ObservedFile* addNewObservedFile( ObservedFile new_observed );
        // Remove a callback, remove and returns the file object if
        // it was the last callback on this object, nullptr if not.
        // The caller has ownership of the object.
        std::shared_ptr<ObservedFile> removeCallback(
                std::shared_ptr<void> callback );

        // Return the watched directory if it is watched, or nullptr
        std::shared_ptr<ObservedDir> watchedDirectory( std::string dir_name );
        // Create a new watched directory for dir_name
        std::shared_ptr<ObservedDir> addWatchedDirectory( std::string dir_name );

      private:
        // List of observed files
        std::list<ObservedFile> observed_files_;

        // List of observed dirs, key-ed by name
        std::map<std::string, std::weak_ptr<ObservedDir>> observed_dirs_;

        // Map the inotify file (including symlinks) wds to the observed file
        std::map<int, ObservedFile*> by_file_wd_;
        // Map the inotify directory wds to the observed files
        std::map<int, ObservedFile*> by_dir_wd_;
    };

    ObservedFileList observed_file_list_;

    // Protects the observed_file_list_
    std::mutex observers_mutex_;

    // Thread
    std::atomic_bool running_;
    std::thread thread_;

    // Only written at initialisation so no protection needed.
    const int inotify_fd;

    // Private member functions
    static void removeNotification( INotifyWatchTower* watch_tower,
            std::shared_ptr<void> notification );
    void run();
};

#endif
