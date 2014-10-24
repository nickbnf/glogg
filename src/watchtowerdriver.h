#ifndef WATCHTOWERDRIVER_H
#define WATCHTOWERDRIVER_H

#include <mutex>

#include "watchtowerlist.h"

class WatchTowerDriver {
  public:
    class FileId { };
    class DirId { };
    class SymlinkId { };

    WatchTowerDriver() {}
    virtual ~WatchTowerDriver() {}

    virtual FileId* addFile( const std::string& file_name ) = 0;
    virtual DirId* addDir( const std::string& file_name ) = 0;
    virtual SymlinkId* addSymlink( const std::string& file_name ) = 0;

    virtual void removeFile( const FileId* ) = 0;
    virtual void removeSymlink( const SymlinkId* ) = 0;

    virtual std::vector<ObservedFile*> waitAndProcessEvents(
            std::shared_ptr<ObservedFileList> list, std::mutex list_mutex ) = 0;
};

#endif
