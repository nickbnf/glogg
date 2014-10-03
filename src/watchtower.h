#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <list>

class WatchTower {
  public:
    // Registration object to implement RAII
    using Registration = std::shared_ptr<void>;

    // Create an empty watchtower
    WatchTower() {};
    // Destroy the object
    virtual ~WatchTower() {};

    // Add a file to the notification list. notification will be called when
    // the file is modified, moved or deleted.
    // Lifetime of the notification is tied to the Registration object returned.
    // Note the notification function is called with a mutex held and in a
    // third party thread, beware of races!
    virtual Registration addFile( const std::string& file_name,
            std::function<void()> notification ) = 0;

  protected:
    // Utility classes

    // List of files and observers
    struct ObservedDir {
        ObservedDir( const std::string this_path ) : path { this_path } {}
        std::string path;
        int dir_wd_;
    };

    struct ObservedFile {
        ObservedFile(
                const std::string& file_name,
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

    // A list of the observed files and directories
    // This class is not thread safe
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
};
