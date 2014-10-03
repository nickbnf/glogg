#include "winwatchtower.h"

WinWatchTower::WinWatchTower() : WatchTower()
{
}

WinWatchTower::~WinWatchTower()
{
}

WatchTower::Registration WinWatchTower::addFile(
        const std::string& file_name,
        std::function<void()> notification )
{
}
