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

// FIXME
#include "inotifywatchtowerdriver.h"

// Utility classes
struct ProtocolInfo {
    // Win32 notification variables
    static const int READ_DIR_CHANGE_BUFFER_SIZE = 4096;

    void* handle_;
    static const unsigned long buffer_length_ = READ_DIR_CHANGE_BUFFER_SIZE;
    char buffer_[buffer_length_];
};

// List of files and observers
struct ObservedDir {
    ObservedDir( const std::string this_path ) : path { this_path } {}

    // Returns the address of the protocol specific informations
    ProtocolInfo* protocolInfo() { return &protocol_info_; }

    std::string path;
    INotifyWatchTowerDriver::DirId dir_id_;
    // Contains data specific to the protocol (inotify/Win32...)
    ProtocolInfo protocol_info_;
};

struct ObservedFile {
    ObservedFile(
            const std::string& file_name,
            std::shared_ptr<void> callback,
            INotifyWatchTowerDriver::FileId file_id,
            INotifyWatchTowerDriver::SymlinkId symlink_id )
        : file_name_( file_name ) {
        addCallback( callback );

        file_id_    = file_id;
        symlink_id_ = symlink_id;
        dir_        = nullptr;
    }

    void addCallback( std::shared_ptr<void> callback ) {
        callbacks.push_back( callback );
    }

    std::string file_name_;
    // List of callbacks for this file
    std::vector<std::shared_ptr<void>> callbacks;

    // watch descriptor for the file itself
    INotifyWatchTowerDriver::FileId file_id_;
    // watch descriptor for the symlink (if file is a symlink)
    INotifyWatchTowerDriver::SymlinkId symlink_id_;

    // link to the dir containing the file
    std::shared_ptr<ObservedDir> dir_;
};

// A list of the observed files and directories
// This class is not thread safe
class ObservedFileList {
    public:
        ObservedFileList() = default;
        ~ObservedFileList() = default;

        ObservedFile* searchByName( const std::string& file_name );
        ObservedFile* searchByFileOrSymlinkWd(
                INotifyWatchTowerDriver::FileId file_id,
                INotifyWatchTowerDriver::SymlinkId symlink_id );
        ObservedFile* searchByDirWdAndName(
                INotifyWatchTowerDriver::DirId id, const char* name );

        ObservedFile* addNewObservedFile( ObservedFile new_observed );
        // Remove a callback, remove and returns the file object if
        // it was the last callback on this object, nullptr if not.
        // The caller has ownership of the object.
        std::shared_ptr<ObservedFile> removeCallback(
                std::shared_ptr<void> callback );

        // Return the watched directory if it is watched, or nullptr
        std::shared_ptr<ObservedDir> watchedDirectory( const std::string& dir_name );
        // Create a new watched directory for dir_name
        std::shared_ptr<ObservedDir> addWatchedDirectory( const std::string& dir_name );

        std::shared_ptr<ObservedDir> watchedDirectoryForFile( const std::string& file_name );
        std::shared_ptr<ObservedDir> addWatchedDirectoryForFile( const std::string& file_name );

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

#endif
