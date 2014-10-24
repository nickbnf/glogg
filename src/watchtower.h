#ifndef WATCHTOWER_H
#define WATCHTOWER_H

#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

#include "watchtowerlist.h"
#include "watchtowerdriver.h"

// Allow the client to register for notification on an arbitrary number
// of files. It will be notified to any change (creation/deletion/modification)
// on those files.
// It is passed a platform specific driver.
class WatchTower {
  public:
    // Registration object to implement RAII
    using Registration = std::shared_ptr<void>;

    // Create an empty watchtower
    WatchTower( std::shared_ptr<WatchTowerDriver> driver );
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
    std::shared_ptr<WatchTowerDriver> driver_;

    // List of files/dirs observed
    ObservedFileList observed_file_list_;

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

#endif
