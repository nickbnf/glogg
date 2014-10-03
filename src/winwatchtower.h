#include "watchtower.h"

class WinWatchTower : public WatchTower {
  public:
    // Create an empty watchtower
    WinWatchTower();
    // Destroy the object
    ~WinWatchTower();

    Registration addFile( const std::string& file_name,
            std::function<void()> notification ) override;
};
