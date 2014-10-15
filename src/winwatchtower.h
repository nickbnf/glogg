#include "watchtower.h"

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iterator>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Utility class

// Encapsulate a directory notification returned by Windows'
// ReadDirectoryChangesW.
class WinNotificationInfo {
  public:
    enum class Action { UNDEF,
        ADDED,
        REMOVED,
        MODIFIED,
        RENAMED_OLD_NAME,
        RENAMED_NEW_NAME };

    WinNotificationInfo() { action_ = Action::UNDEF; }
    WinNotificationInfo( Action action, std::wstring file_name )
    { action_ = action; file_name_ = file_name; }

    Action action() const { return action_; }
    std::wstring fileName() const { return file_name_; }

  private:
    Action action_;
    std::wstring file_name_;
};

class WinNotificationInfoList {
  public:
    WinNotificationInfoList( const char* buffer, size_t buffer_size );

    // Iterator
    class iterator : std::iterator<std::input_iterator_tag, WinNotificationInfo> {
      public:
        iterator( WinNotificationInfoList* list, const char* position )
        { list_ = list; position_ = position; }

        const WinNotificationInfo& operator*() const
        { return list_->current_notification_; }

        const WinNotificationInfo* operator->() const
        { return &( list_->current_notification_ ); }

        const WinNotificationInfo& operator++() {
            position_ = list_->advanceToNext();
            return list_->current_notification_;
        }

        WinNotificationInfo operator++( int ) {
            WinNotificationInfo tmp { list_->current_notification_ };
            operator++();
            return tmp;
        }

        bool operator!=( const iterator& other ) const {
            return ( list_ != other.list_ ) || ( position_ != other.position_ );
        }

      private:
        WinNotificationInfoList* list_;
        const char* position_;
    };

    iterator begin() { return iterator( this, pointer_ ); }
    iterator end() { return iterator( this, nullptr ); }

  private:
    const char* advanceToNext();

    // Current notification (in the byte stream)
    const char* pointer_;
    // Next notification (in the byte stream)
    const char* next_;
    WinNotificationInfo current_notification_;

    const char* updateCurrentNotification( const char* new_position );
};

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
    HANDLE     hCompPort_;
    OVERLAPPED overlapped_;
    unsigned long buffer_length_;

    // List of followed files/directory
    // (access is only done by the WatchTower thread)
    ObservedFileList file_list_;

    void addFile( const std::string& file_name );
    void run();
};
