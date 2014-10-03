#include "watchtower.h"

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class WinWatchTower : public WatchTower {
  public:
    // Create an empty watchtower
    WinWatchTower();
    // Destroy the object
    ~WinWatchTower();

    Registration addFile( const std::string& file_name,
            std::function<void()> notification ) override;
  private:
    // An action which will be picked up by the worker thread.
    class Action {
      public:
        Action( std::function<void()> function ) : function_ { function } {}
        ~Action() {}

        void operator()( HANDLE completion_port ) { function_(); }

      private:
        std::function<void()> function_;
    };

    // Thread
    std::atomic_bool running_;
    std::thread thread_;

    // Action
    std::mutex action_mutex_;
    std::condition_variable action_done_cv_;
    std::shared_ptr<Action> scheduled_action_ = nullptr;

    // Win32 notification variables
    static const int READ_DIR_CHANGE_BUFFER_SIZE = 4096;

    HANDLE     hCompPort_;
    CHAR       buffer_[READ_DIR_CHANGE_BUFFER_SIZE];//buffer for ReadDirectoryChangesW
    OVERLAPPED overlapped_;
    unsigned long buffer_length_;

    // List of followed files/directory
    // (access is only done by the WatchTower thread)
    ObservedFileList file_list_;

    void addFile( const std::string& file_name );
    void run();
};
