/*
 * Copyright (C) 2015 Nicolas Bonnefon and other contributors
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

#ifndef WATCHTOWER_H
#define WATCHTOWER_H

#include "config.h"

#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

#include "watchtowerlist.h"

// Allow the client to register for notification on an arbitrary number
// of files. It will be notified to any change (creation/deletion/modification)
// on those files.
// It is passed a platform specific driver.

// FIXME: Where to put that so it is not dependant
// Registration object to implement RAII
#ifdef HAS_TEMPLATE_ALIASES
using Registration = std::shared_ptr<void>;
#else
typedef std::shared_ptr<void> Registration;
#endif

template<typename Driver>
class WatchTower {
  public:
    // Create an empty watchtower
    WatchTower();
    // Destroy the object
    ~WatchTower();

    // Set the polling interval (in ms)
    // 0 disables polling and is the default
    void setPollingInterval( uint32_t interval_ms );

    // Add a file to the notification list. notification will be called when
    // the file is modified, moved or deleted.
    // Lifetime of the notification is tied to the Registration object returned.
    // Note the notification function is called with a mutex held and in a
    // third party thread, beware of races!
    Registration addFile( const std::string& file_name,
            std::function<void()> notification );

    // Number of watched directories (for tests)
    unsigned int numberWatchedDirectories() const;

  private:
    // The driver (parametrised)
    Driver driver_;

    // List of files/dirs observed
    ObservedFileList<Driver> observed_file_list_;

    // Protects the observed_file_list_
    std::mutex observers_mutex_;

    // Polling interval (0 disables polling)
    uint32_t polling_interval_ms_ = 0;

    // Thread
    std::atomic_bool running_;
    std::thread thread_;

    // Exist as long as the onject exists, to ensure observers won't try to
    // call us if we are dead.
    std::shared_ptr<void> heartBeat_;

    // Private member functions
    std::tuple<typename Driver::FileId, typename Driver::SymlinkId>
        addFileToDriver( const std::string& );
    static void removeNotification( WatchTower* watch_tower,
            std::shared_ptr<void> notification );
    void run();
};

// Class template implementation

#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"

namespace {
    bool isSymLink( const std::string& file_name );
    std::string directory_path( const std::string& path );
};

template <typename Driver>
WatchTower<Driver>::WatchTower()
    : driver_(), thread_(),
    heartBeat_(std::shared_ptr<void>((void*) 0xDEADC0DE, [] (void*) {}))
{
    running_ = true;
    thread_ = std::thread( &WatchTower::run, this );
}

template <typename Driver>
WatchTower<Driver>::~WatchTower()
{
    running_ = false;
    driver_.interruptWait();
    thread_.join();
}

template <typename Driver>
void WatchTower<Driver>::setPollingInterval( uint32_t interval_ms )
{
    if ( polling_interval_ms_ != interval_ms ) {
        polling_interval_ms_ = interval_ms;
        // Break out of the wait
        if ( polling_interval_ms_ > 0 )
            driver_.interruptWait();
    }
}

template <typename Driver>
Registration WatchTower<Driver>::addFile(
        const std::string& file_name,
        std::function<void()> notification )
{
    LOG(logDEBUG) << "WatchTower::addFile " << file_name;

    std::weak_ptr<void> weakHeartBeat(heartBeat_);

    std::lock_guard<std::mutex> lock( observers_mutex_ );

    auto existing_observed_file =
        observed_file_list_.searchByName( file_name );

    std::shared_ptr<std::function<void()>> ptr( new std::function<void()>(std::move( notification )) );

    if ( ! existing_observed_file )
    {
        typename Driver::FileId file_id;
        typename Driver::SymlinkId symlink_id;

        std::tie( file_id, symlink_id ) = addFileToDriver( file_name );
        auto new_file = observed_file_list_.addNewObservedFile(
                ObservedFile<Driver>( file_name, ptr, file_id, symlink_id ) );

        auto dir = observed_file_list_.watchedDirectoryForFile( file_name );
        if ( ! dir ) {
            LOG(logDEBUG) << "WatchTower::addFile dir for " << file_name
                << " not watched, adding...";

            dir = observed_file_list_.addWatchedDirectoryForFile( file_name,
                    [this, weakHeartBeat] (ObservedDir<Driver>* dir) {
                        if ( auto heart_beat = weakHeartBeat.lock() ) {
                            driver_.removeDir( dir->dir_id_ );
                        } } );

            dir->dir_id_ = driver_.addDir( dir->path );

            if ( ! dir->dir_id_.valid() ) {
                LOG(logWARNING) << "WatchTower::addFile driver failed to add dir";
                dir = nullptr;
            }
        }
        else {
            LOG(logDEBUG) << "WatchTower::addFile Found exisiting watch for dir " << file_name;
        }

        // Associate the dir to the file
        if ( dir )
            new_file->dir_ = dir;
    }
    else
    {
        LOG(logDEBUG) << "WatchTower::addFile add extra callback for already monitored " << file_name;
        existing_observed_file->addCallback( ptr );
    }

    // Returns a shared pointer that removes its own entry
    // from the list of watched stuff when it goes out of scope!
    // Uses a custom deleter to do the work.
    return std::shared_ptr<void>( 0x0, [this, ptr, weakHeartBeat] (void*) {
            if ( auto heart_beat = weakHeartBeat.lock() )
                WatchTower<Driver>::removeNotification( this, ptr );
            } );
}

template <typename Driver>
unsigned int WatchTower<Driver>::numberWatchedDirectories() const
{
    return observed_file_list_.numberWatchedDirectories();
}

//
// Private functions
//

// Add the passed file name to the driver, returning the file and symlink id
template <typename Driver>
std::tuple<typename Driver::FileId, typename Driver::SymlinkId>
WatchTower<Driver>::addFileToDriver( const std::string& file_name )
{
    typename Driver::SymlinkId symlink_id;
    auto file_id = driver_.addFile( file_name );

    if ( isSymLink( file_name ) )
    {
        // We want to follow the name (as opposed to the inode)
        // so we watch the symlink as well.
        symlink_id = driver_.addSymlink( file_name );
    }

    return std::make_tuple( file_id, symlink_id );
}

// Called by the dtor for a registration object
template <typename Driver>
void WatchTower<Driver>::removeNotification(
        WatchTower* watch_tower, std::shared_ptr<void> notification )
{
    LOG(logDEBUG) << "WatchTower::removeNotification";

    std::lock_guard<std::mutex> lock( watch_tower->observers_mutex_ );

    auto file =
        watch_tower->observed_file_list_.removeCallback( notification );

    if ( file )
    {
        LOG(logDEBUG) << "WatchTower::removeNotification - remove the file";
        watch_tower->driver_.removeFile( file->file_id_ );
        watch_tower->driver_.removeSymlink( file->symlink_id_ );
    }
}

// Run in its own thread
template <typename Driver>
void WatchTower<Driver>::run()
{
    while ( running_ ) {
        std::unique_lock<std::mutex> lock( observers_mutex_ );

        std::vector<ObservedFile<Driver>*> files_needing_readding;

        auto files = driver_.waitAndProcessEvents(
                &observed_file_list_, &lock, &files_needing_readding, polling_interval_ms_ );
        LOG(logDEBUG) << "WatchTower::run: waitAndProcessEvents returned "
            << files.size() << " files, " << files_needing_readding.size()
            << " needing re-adding";

        for ( auto file: files_needing_readding ) {
            // A file 'needing readding' has the same name,
            // but probably a different inode, so it needs
            // to be readded for some drivers that rely on the
            // inode (e.g. inotify)
            driver_.removeFile( file->file_id_ );
            driver_.removeSymlink( file->symlink_id_ );

            std::tie( file->file_id_, file->symlink_id_ ) =
                addFileToDriver( file->file_name_ );
        }

        for ( auto file: files ) {
            for ( auto observer: file->callbacks ) {
                LOG(logDEBUG) << "WatchTower::run: notifying the client!";
                // Here we have to cast our generic pointer back to
                // the function pointer in order to perform the call
                const std::shared_ptr<std::function<void()>> fptr =
                    std::static_pointer_cast<std::function<void()>>( observer );
                // The observer is called with the mutex held,
                // Let's hope it doesn't do anything too funky.
                (*fptr)();

                file->markAsChanged();
            }
        }

        if ( polling_interval_ms_ > 0 ) {
            // Also call files that have not been called for a while
            for ( auto file: observed_file_list_ ) {
                uint32_t ms_since_last_check =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - file->timeForLastCheck() ).count();
                if ( ( ms_since_last_check > polling_interval_ms_ ) && file->hasChanged() ) {
                    LOG(logDEBUG) << "WatchTower::run: " << file->file_name_;
                    for ( auto observer: file->callbacks ) {
                        LOG(logDEBUG) << "WatchTower::run: notifying the client because of a timeout!";
                        // Here we have to cast our generic pointer back to
                        // the function pointer in order to perform the call
                        const std::shared_ptr<std::function<void()>> fptr =
                            std::static_pointer_cast<std::function<void()>>( observer );
                        // The observer is called with the mutex held,
                        // Let's hope it doesn't do anything too funky.
                        (*fptr)();
                    }
                    file->markAsChanged();
                }
            }
        }
    }
}

namespace {
    bool isSymLink( const std::string& file_name )
    {
#ifdef HAVE_SYMLINK
        struct stat buf;

        lstat( file_name.c_str(), &buf );
        return ( S_ISLNK(buf.st_mode) );
#else
        return false;
#endif
    }
};

#endif
