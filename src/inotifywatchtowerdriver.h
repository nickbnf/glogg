#ifndef INOTIFYWATCHTOWERDRIVER_H
#define INOTIFYWATCHTOWERDRIVER_H

#include "watchtowerdriver.h"

class INotifyWatchTowerDriver : public WatchTowerDriver {
  public:
    class INotifyFileId : public FileId {
    };
    class INotifyDirId : public DirId {
    };
    class INotifySymlinkId : public SymlinkId {
    };

    INotifyWatchTowerDriver();
    ~INotifyWatchTowerDriver() override {}

    // These functions returns covariant types, which unfortunately
    // must be passed by plain pointers.
    INotifyFileId* addFile( const std::string& file_name ) override;
    INotifyDirId* addSymlink( const std::String& file_name ) override;
    INotifySymlinkId* addDir( const std::string& file_name ) override;

    void removeFile( const INotifyFileId* file_id ) override;
    void removeSymlink( const INotifySymlinkId* symlink_id ) override;

    std::vector<ObservedFile*> waitAndProcessEvents( std::shared<> list,
            std::mutex list_mutex ) override;
};

#endif
