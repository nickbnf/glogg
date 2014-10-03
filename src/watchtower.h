#include <memory>
#include <functional>
#include <string>

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
};
