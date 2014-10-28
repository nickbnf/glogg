#ifndef INOTIFYWATCHTOWERDRIVER_H
#define INOTIFYWATCHTOWERDRIVER_H

#include <memory>
#include <mutex>
#include <vector>

template <typename Driver>
class ObservedFile;
template <typename Driver>
class ObservedFileList;

class INotifyWatchTowerDriver {
  public:
    class FileId {
      public:
        friend class INotifyWatchTowerDriver;

        FileId() { wd_ = -1; }
        bool operator==( const FileId& other ) const
        { return wd_ == other.wd_; }
      private:
        FileId( int wd ) { wd_ = wd; }
        int wd_;
    };
    class DirId {
      public:
        friend class INotifyWatchTowerDriver;

        DirId() { wd_ = -1; }
        bool operator==( const DirId& other ) const
        { return wd_ == other.wd_; }
      private:
        DirId( int wd ) { wd_ = wd; }
        int wd_;
    };
    class SymlinkId {
      public:
        friend class INotifyWatchTowerDriver;

        SymlinkId() { wd_ = -1; }
        bool operator==( const SymlinkId& other ) const
        { return wd_ == other.wd_; }
      private:
        SymlinkId( int wd ) { wd_ = wd; }
        int wd_;
    };

    using INotifyObservedFile = ObservedFile<INotifyWatchTowerDriver>;
    using INotifyObservedFileList = ObservedFileList<INotifyWatchTowerDriver>;

    // Default constructor
    INotifyWatchTowerDriver();
    ~INotifyWatchTowerDriver();

    // No copy/assign/move please
    INotifyWatchTowerDriver( const INotifyWatchTowerDriver& ) = delete;
    INotifyWatchTowerDriver& operator=( const INotifyWatchTowerDriver& ) = delete;
    INotifyWatchTowerDriver( const INotifyWatchTowerDriver&& ) = delete;
    INotifyWatchTowerDriver& operator=( const INotifyWatchTowerDriver&& ) = delete;

    FileId addFile( const std::string& file_name );
    SymlinkId addSymlink( const std::string& file_name );
    DirId addDir( const std::string& file_name );

    void removeFile( const FileId& file_id );
    void removeSymlink( const SymlinkId& symlink_id );

    std::vector<INotifyObservedFile*> waitAndProcessEvents(
            INotifyObservedFileList* list,
            std::mutex* list_mutex );
    void interruptWait();

  private:
    // Only written at initialisation so no protection needed.
    const int inotify_fd_;

    // Breaking pipe
    int breaking_pipe_read_fd_;
    int breaking_pipe_write_fd_;

    // Private member functions
    size_t processINotifyEvent( const struct inotify_event* event,
            INotifyObservedFileList* list,
            std::mutex* list_mutex,
            std::vector<INotifyObservedFile*>* files_to_notify );
};

#endif
