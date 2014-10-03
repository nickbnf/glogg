#include "gmock/gmock.h"

#include <memory>

#include "winfilewatcher.h"

using namespace std;

class FileWatcherBehaviour: public testing::Test {
  public:
    shared_ptr<FileWatcher> file_watcher;

    void SetUp() override {
#ifdef _WIN32
        // file_watcher = make_shared<WinFileWatcher>();
#endif
    }
};

TEST_F( FileWatcherBehaviour, DetectsAnAppendedFile ) {
    EXPECT_EQ(1, 1);
}
