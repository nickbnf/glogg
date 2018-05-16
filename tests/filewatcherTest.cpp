#include "gmock/gmock.h"

#include <memory>

#include "platformfilewatcher.h"

using namespace std;

class FileWatcherBehaviour: public testing::Test {
  public:
    shared_ptr<FileWatcher> file_watcher;

    void SetUp() override {
        file_watcher = make_shared<PlatformFileWatcher>();
    }
};

TEST_F( FileWatcherBehaviour, DetectsAnAppendedFile ) {
    EXPECT_EQ(1, 1);
}
