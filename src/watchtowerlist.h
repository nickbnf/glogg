#ifndef WATCHTOWERLIST_H
#define WATCHTOWERLIST_H

// Utility classes for the WatchTower implementations

#include <functional>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <memory>
#include <algorithm>
#include <chrono>

#include "log.h"

// Utility classes
struct ProtocolInfo {
    // Win32 notification variables
    static const int READ_DIR_CHANGE_BUFFER_SIZE = 4096;

    void* handle_;
    static const unsigned long buffer_length_ = READ_DIR_CHANGE_BUFFER_SIZE;
    char buffer_[buffer_length_];
};

// List of files and observers
template <typename Driver>
struct ObservedDir {
    ObservedDir( const std::string this_path ) : path { this_path } {}

    // Returns the address of the protocol specific informations
    ProtocolInfo* protocolInfo() { return &protocol_info_; }

    std::string path;
    typename Driver::DirId dir_id_;
    // Contains data specific to the protocol (inotify/Win32...)
    ProtocolInfo protocol_info_;
};

template <typename Driver>
struct ObservedFile {
    ObservedFile(
            const std::string& file_name,
            std::shared_ptr<void> callback,
            typename Driver::FileId file_id,
            typename Driver::SymlinkId symlink_id )
        : file_name_( file_name ) {
        addCallback( callback );

        file_id_    = file_id;
        symlink_id_ = symlink_id;
        dir_        = nullptr;

        markAsChanged();
    }

    void addCallback( std::shared_ptr<void> callback ) {
        callbacks.push_back( callback );
    }

    // Records the file has changed
    void markAsChanged() {
        change_token_.readFromFile( file_name_ );
        last_check_time_ = std::chrono::steady_clock::now();
    }

    // Returns whether a file has changed
    // (for polling)
    bool hasChanged() {
        typename Driver::FileChangeToken new_token( file_name_ );
        last_check_time_ = std::chrono::steady_clock::now();
        return change_token_ != new_token;
    }

    std::chrono::steady_clock::time_point timeForLastCheck() {
        return last_check_time_;
    }

    std::string file_name_;
    // List of callbacks for this file
    std::vector<std::shared_ptr<void>> callbacks;

    // watch descriptor for the file itself
    typename Driver::FileId file_id_;
    // watch descriptor for the symlink (if file is a symlink)
    typename Driver::SymlinkId symlink_id_;

    // link to the dir containing the file
    std::shared_ptr<ObservedDir<Driver>> dir_;

    // token to identify modification
    // (the token change when the file is modified, this is used when polling)
    typename Driver::FileChangeToken change_token_;

    // Last time a check has been done
    std::chrono::steady_clock::time_point last_check_time_ =
        std::chrono::steady_clock::time_point::min();
};

// A list of the observed files and directories
// This class is not thread safe
template<typename Driver>
class ObservedFileList {
    public:
        ObservedFileList() :
            // Use an empty deleter since we don't really own this.
            heartBeat_ { this, [] (void*) {} }
            { }
        ObservedFileList( const ObservedFileList& ) = delete;
        ~ObservedFileList() = default;

        // The functions return a pointer to the existing file (if exists)
        // but keep ownership of the object.
        ObservedFile<Driver>* searchByName( const std::string& file_name );
        ObservedFile<Driver>* searchByFileOrSymlinkWd(
                typename Driver::FileId file_id,
                typename Driver::SymlinkId symlink_id );
        ObservedFile<Driver>* searchByDirWdAndName(
                typename Driver::DirId id, const char* name );
        std::vector<ObservedFile<Driver>*> searchByDirWd(
                typename Driver::DirId id );

        // Add a new file, the list returns a pointer to the added file,
        // but has ownership of the file.
        ObservedFile<Driver>* addNewObservedFile( ObservedFile<Driver> new_observed );
        // Remove a callback, remove and returns the file object if
        // it was the last callback on this object, nullptr if not.
        // The caller has ownership of the object.
        std::shared_ptr<ObservedFile<Driver>> removeCallback(
                std::shared_ptr<void> callback );

        // Return the watched directory if it is watched, or nullptr
        std::shared_ptr<ObservedDir<Driver>> watchedDirectory( const std::string& dir_name );
        // Create a new watched directory for dir_name, the client is passed
        // shared ownership and have to keep the shared_ptr (the list only
        // maintain a weak link).
        // The remove notification is called just before the reference to
        // the directory is destroyed.
        std::shared_ptr<ObservedDir<Driver>> addWatchedDirectory(
            const std::string& dir_name,
            std::function<void( ObservedDir<Driver>* )> remove_notification );

        // Similar to previous functions but extract the name of the
        // directory from the file name.
        std::shared_ptr<ObservedDir<Driver>> watchedDirectoryForFile( const std::string& file_name );
        std::shared_ptr<ObservedDir<Driver>> addWatchedDirectoryForFile( const std::string& file_name,
                std::function<void( ObservedDir<Driver>* )> remove_notification );

        // Removal of directories is done when there is no shared reference
        // left (RAII)

        // Number of watched directories (for tests)
        unsigned int numberWatchedDirectories() const;

        // Iterator
        template<typename Container>
        class iterator : std::iterator<std::input_iterator_tag, ObservedFile<Driver>> {
          public:
            iterator( Container* list,
                   const typename Container::iterator& iter )
            { list_ = list; pos_ = iter; }

            iterator operator++()
            { ++pos_; return *this; }

            bool operator==( const iterator& other )
            { return ( pos_ == other.pos_ ); }

            bool operator!=( const iterator& other )
            { return ! operator==( other ); }

            typename Container::iterator operator*()
            { return pos_; }

          private:
            Container* list_;
            typename Container::iterator pos_;
        };

        iterator<std::list<ObservedFile<Driver>>> begin()
        { return iterator<std::list<ObservedFile<Driver>>>( &observed_files_, observed_files_.begin() ); }
        iterator<std::list<ObservedFile<Driver>>> end()
        { return iterator<std::list<ObservedFile<Driver>>>( &observed_files_, observed_files_.end() ); }

    private:
        // List of observed files
        std::list<ObservedFile<Driver>> observed_files_;

        // List of observed dirs, key-ed by name
        std::map<std::string, std::weak_ptr<ObservedDir<Driver>>> observed_dirs_;

        // Map the inotify file (including symlinks) wds to the observed file
        std::map<int, ObservedFile<Driver>*> by_file_wd_;
        // Map the inotify directory wds to the observed files
        std::map<int, ObservedFile<Driver>*> by_dir_wd_;

        // A shared_ptr to self.
        // weak_ptr's from this can check if the list is still alive.
        // This probably shouldn't be copied.
        std::shared_ptr<ObservedFileList> heartBeat_;

        // Clean all reference to any expired directory
        void cleanRefsToExpiredDirs();
};

namespace {
    std::string directory_path( const std::string& path );
};

// ObservedFileList class
template <typename Driver>
ObservedFile<Driver>* ObservedFileList<Driver>::searchByName(
        const std::string& file_name )
{
    // Look for an existing observer on this file
    auto existing_observer = observed_files_.begin();
    for ( ; existing_observer != observed_files_.end(); ++existing_observer )
    {
        if ( existing_observer->file_name_ == file_name )
        {
            LOG(logDEBUG) << "Found " << file_name;
            break;
        }
    }

    if ( existing_observer != observed_files_.end() )
        return &( *existing_observer );
    else
        return nullptr;
}

template <typename Driver>
ObservedFile<Driver>* ObservedFileList<Driver>::searchByFileOrSymlinkWd(
        typename Driver::FileId file_id,
        typename Driver::SymlinkId symlink_id )
{
    auto result = find_if( observed_files_.begin(), observed_files_.end(),
            [file_id, symlink_id] (ObservedFile<Driver> file) -> bool {
                return ( file_id == file.file_id_ ) ||
                    ( symlink_id == file.symlink_id_ );
                } );

    if ( result != observed_files_.end() )
        return &( *result );
    else
        return nullptr;
}

template <typename Driver>
ObservedFile<Driver>* ObservedFileList<Driver>::searchByDirWdAndName(
        typename Driver::DirId id, const char* name )
{
    auto dir = find_if( observed_dirs_.begin(), observed_dirs_.end(),
            [id] (std::pair<std::string,std::weak_ptr<ObservedDir<Driver>>> d) -> bool {
            if ( auto dir = d.second.lock() ) {
                return ( id == dir->dir_id_ );
            }
            else {
                return false; } } );

    if ( dir != observed_dirs_.end() ) {
        std::string path = dir->first + "/" + name;

        // LOG(logDEBUG) << "Testing path: " << path;

        // Looking for the path in the files we are watching
        return searchByName( path );
    }
    else {
        return nullptr;
    }
}

template <typename Driver>
std::vector<ObservedFile<Driver>*> ObservedFileList<Driver>::searchByDirWd(
        typename Driver::DirId id )
{
    std::vector<ObservedFile<Driver>*> result;

    auto dir = find_if( observed_dirs_.begin(), observed_dirs_.end(),
            [id] (std::pair<std::string,std::weak_ptr<ObservedDir<Driver>>> d) -> bool {
            if ( auto dir = d.second.lock() ) {
                return ( id == dir->dir_id_ );
            }
            else {
                return false; } } );

    if ( dir != observed_dirs_.end() ) {
        for ( auto i = observed_files_.begin(); i != observed_files_.end(); ++i ) {
            if ( auto d = dir->second.lock() ) {
                if ( d.get() == i->dir_.get() ) {
                    result.push_back( &(*i) );
                }
            }
        }
    }

    return result;
}

template <typename Driver>
ObservedFile<Driver>* ObservedFileList<Driver>::addNewObservedFile(
        ObservedFile<Driver> new_observed )
{
    auto new_file = observed_files_.insert( std::begin( observed_files_ ), new_observed );

    return &( *new_file );
}

template <typename Driver>
std::shared_ptr<ObservedFile<Driver>> ObservedFileList<Driver>::removeCallback(
        std::shared_ptr<void> callback )
{
    std::shared_ptr<ObservedFile<Driver>> returned_file = nullptr;

    for ( auto observer = std::begin( observed_files_ );
            observer != std::end( observed_files_ ); )
    {
        std::vector<std::shared_ptr<void>>& callbacks = observer->callbacks;
        callbacks.erase( std::remove(
                    std::begin( callbacks ), std::end( callbacks ), callback ),
                std::end( callbacks ) );

        /* See if all notifications have been deleted for this file */
        if ( callbacks.empty() ) {
            LOG(logDEBUG) << "Empty notification list for " << observer->file_name_
                << ", removing the watched file";
            returned_file = std::make_shared<ObservedFile<Driver>>( *observer );
            observer = observed_files_.erase( observer );
        }
        else {
            ++observer;
        }
    }

    return returned_file;
}

template <typename Driver>
std::shared_ptr<ObservedDir<Driver>> ObservedFileList<Driver>::watchedDirectory(
        const std::string& dir_name )
{
    std::shared_ptr<ObservedDir<Driver>> dir = nullptr;

    if ( observed_dirs_.find( dir_name ) != std::end( observed_dirs_ ) )
        dir = observed_dirs_[ dir_name ].lock();

    return dir;
}

template <typename Driver>
std::shared_ptr<ObservedDir<Driver>> ObservedFileList<Driver>::addWatchedDirectory(
        const std::string& dir_name,
        std::function<void( ObservedDir<Driver>* )> remove_notification )
{
    std::weak_ptr<ObservedFileList> weakHeartBeat(heartBeat_);

    std::shared_ptr<ObservedDir<Driver>> dir = {
            new ObservedDir<Driver>( dir_name ),
            [remove_notification, weakHeartBeat] (ObservedDir<Driver>* d) {
                if ( auto list = weakHeartBeat.lock() ) {
                    remove_notification( d );
                    list->cleanRefsToExpiredDirs();
                }
                delete d; } };

    observed_dirs_[ dir_name ] = std::weak_ptr<ObservedDir<Driver>>( dir );

    return dir;
}

template <typename Driver>
std::shared_ptr<ObservedDir<Driver>> ObservedFileList<Driver>::watchedDirectoryForFile(
        const std::string& file_name )
{
    return watchedDirectory( directory_path( file_name ) );
}

template <typename Driver>
std::shared_ptr<ObservedDir<Driver>> ObservedFileList<Driver>::addWatchedDirectoryForFile(
        const std::string& file_name,
        std::function<void( ObservedDir<Driver>* )> remove_notification )
{
    return addWatchedDirectory( directory_path( file_name ),
            remove_notification );
}

template <typename Driver>
unsigned int ObservedFileList<Driver>::numberWatchedDirectories() const
{
    return observed_dirs_.size();
}

// Private functions
template <typename Driver>
void ObservedFileList<Driver>::cleanRefsToExpiredDirs()
{
    for ( auto it = std::begin( observed_dirs_ );
            it != std::end( observed_dirs_ ); )
    {
        if ( it->second.expired() ) {
            it = observed_dirs_.erase( it );
        }
        else {
            ++it;
        }
    }
}

namespace {
    std::string directory_path( const std::string& path )
    {
        size_t slash_pos = path.rfind( '/' );

#ifdef _WIN32
        if ( slash_pos == std::string::npos ) {
            slash_pos = path.rfind( '\\' );
        }

        // We need to include the final slash on Windows
        ++slash_pos;
#endif

        return std::string( path, 0, slash_pos );
    }
};
#endif
