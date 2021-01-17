#ifndef SYNCPIPE_H
#define SYNCPIPE_H

#include <deque>
#include <string>
#include <mutex>
#include <optional>
#include <condition_variable>

using namespace std;

class SyncPipe
{
public:
    SyncPipe();
    void push(char ch);
    string pop();
private:
    deque<char> mPipe;
    mutex mMutex;
    condition_variable dataAvailable;
};

#endif // SYNCPIPE_H
