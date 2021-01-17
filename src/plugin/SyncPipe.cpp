#include "SyncPipe.h"
#include <sstream>
#include <chrono>

SyncPipe::SyncPipe()
{

}

void SyncPipe::push(char ch)
{
    unique_lock<mutex> ul(mMutex);

    mPipe.push_front(ch);
    dataAvailable.notify_one();
}

string SyncPipe::pop()
{
    string ret;
    stringstream ss;
    unique_lock<mutex> ul(mMutex);

    dataAvailable.wait_for(ul, chrono::milliseconds(500), [this](){ return not mPipe.empty(); });

    while(not mPipe.empty()){
        ss << mPipe.back();
        mPipe.pop_back();
    }

    ret = ss.str();

    return ret;
}
