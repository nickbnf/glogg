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
#if __GNUC_MINOR__ < 7
typedef std::shared_ptr<void> Registration;
#else
using Registration = std::shared_ptr<void>;
#endif

template<typename Driver>
class WatchTower {
  public:
    // Create an empty watchtower
    WatchTower();
    // Destroy the object
    ~WatchTower();

    // Add a file to the notification list. notification will be called when
    // the file is modified, moved or deleted.
    // Lifetime of the notification is tied to the Registration object returned.
    // Note the notification function is called with a mutex held and in a
    // third party thread, beware of races!
    Registration addFile( const std::string& file_name,
            std::function<void()> notification );

  private:
    // The driver (parametrised)
    Driver driver_;

    // List of files/dirs observed
    ObservedFileList<Driver> observed_file_list_;

    // Protects the observed_file_list_
    std::mutex observers_mutex_;

    // Thread
    std::atomic_bool running_;
    std::thread thread_;

    // Exist as long as the onject exists, to ensure observers won't try to
    // call us if we are dead.
    std::shared_ptr<void> heartBeat_;

    // Private member functions
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
    : thread_(), driver_(),
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
Registration WatchTower<Driver>::addFile(
        const std::string& file_name,
        std::function<void()> notification )
{
    // LOG(logDEBUG) << "WatchTower::addFile " << file_name;

    std::lock_guard<std::mutex> lock( observers_mutex_ );

    auto existing_observed_file =
        observed_file_list_.searchByName( file_name );

    std::shared_ptr<std::function<void()>> ptr( new std::function<void()>(std::move( notification )) );

    if ( ! existing_observed_file )
    {
        typename Driver::SymlinkId symlink_id;

        auto file_id = driver_.addFile( file_name );

        if ( isSymLink( file_name ) )
        {
            // We want to follow the name (as opposed to the inode)
            // so we watch the symlink as well.
            symlink_id = driver_.addSymlink( file_name );
        }

        auto new_file = observed_file_list_.addNewObservedFile(
                ObservedFile<Driver>( file_name, ptr, file_id, symlink_id ) );

        auto dir = observed_file_list_.watchedDirectoryForFile( file_name );
        if ( ! dir )
        {
            LOG(logDEBUG) << "WatchTower::addFile dir for " << file_name
                << " not watched, adding...";
            dir = observed_file_list_.addWatchedDirectoryForFile( file_name );

            dir->dir_id_ = driver_.addDir( dir->path );
        }

        // Associate the dir to the file
        new_file->dir_ = dir;

        LOG(logDEBUG) << "dir ref count is " << dir.use_count();
    }
    else
    {
        existing_observed_file->addCallback( ptr );
    }

    std::weak_ptr<void> weakHeartBeat(heartBeat_);

    // Returns a shared pointer that removes its own entry
    // from the list of watched stuff when it goes out of scope!
    // Uses a custom deleter to do the work.
    return std::shared_ptr<void>( 0x0, [this, ptr, weakHeartBeat] (void*) {
            if ( auto heart_beat = weakHeartBeat.lock() )
                WatchTower<Driver>::removeNotification( this, ptr );
            } );
}

//
// Private functions
//

// Called by the dtor for a registration object
template<typename Driver>
void WatchTower<Driver>::removeNotification(
        WatchTower* watch_tower, std::shared_ptr<void> notification )
{
    LOG(logDEBUG) << "WatchTower::removeNotification";

    std::lock_guard<std::mutex> lock( watch_tower->observers_mutex_ );

    auto file =
        watch_tower->observed_file_list_.removeCallback( notification );

    if ( file )
    {
        watch_tower->driver_.removeFile( file->file_id_ );
        watch_tower->driver_.removeSymlink( file->symlink_id_ );
    }
}

// Run in its own thread
template <typename Driver>
void WatchTower<Driver>::run()
{
    while ( running_ ) {
        auto files = driver_.waitAndProcessEvents(
                &observed_file_list_, &observers_mutex_ );

        for ( auto file: files ) {
            for ( auto observer: file->callbacks ) {
                // Here we have to cast our generic pointer back to
                // the function pointer in order to perform the call
                const std::shared_ptr<std::function<void()>> fptr =
                    std::static_pointer_cast<std::function<void()>>( observer );
                // The observer is called with the mutex held,
                // Let's hope it doesn't do anything too funky.
                (*fptr)();
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
