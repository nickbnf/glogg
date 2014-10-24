#ifndef INOTIFYWATCHTOWERDRIVER_H
#define INOTIFYWATCHTOWERDRIVER_H

#include <memory>
#include <mutex>
#include <vector>

class ObservedFile;
class ObservedFileList;

class INotifyWatchTowerDriver {
  public:
    class FileId {
      public:
        friend class INotifyWatchTowerDriver;

        FileId() {}
        bool operator==( const FileId& other ) const
        { return wd_ == other.wd_; }
      private:
        FileId( int wd ) { wd_ = wd; }
        int wd_ = -1;
    };
    class DirId {
      public:
        friend class INotifyWatchTowerDriver;

        DirId() {}
        bool operator==( const DirId& other ) const
        { return wd_ == other.wd_; }
      private:
        DirId( int wd ) { wd_ = wd; }
        int wd_ = -1;
    };
    class SymlinkId {
      public:
        friend class INotifyWatchTowerDriver;

        SymlinkId() {}
        bool operator==( const SymlinkId& other ) const
        { return wd_ == other.wd_; }
      private:
        SymlinkId( int wd ) { wd_ = wd; }
        int wd_ = -1;
    };

    INotifyWatchTowerDriver();

    FileId addFile( const std::string& file_name );
    SymlinkId addSymlink( const std::string& file_name );
    DirId addDir( const std::string& file_name );

    void removeFile( const FileId& file_id );
    void removeSymlink( const SymlinkId& symlink_id );

    std::vector<ObservedFile*> waitAndProcessEvents(
            ObservedFileList* list,
            std::mutex* list_mutex );

  private:
    // Only written at initialisation so no protection needed.
    const int inotify_fd_;

    // Private member functions
    size_t processINotifyEvent( const struct inotify_event* event,
            ObservedFileList* list,
            std::mutex* list_mutex,
            std::vector<ObservedFile*>* files_to_notify );
};

#endif
