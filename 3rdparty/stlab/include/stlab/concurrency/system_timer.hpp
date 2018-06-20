/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_CONCURRENCY_SYSTEM_TIMER_HPP
#define STLAB_CONCURRENCY_SYSTEM_TIMER_HPP

/**************************************************************************************************/

#include <stlab/concurrency/config.hpp>

#include <chrono>
#include <functional>

#if STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_LIBDISPATCH
#include <dispatch/dispatch.h>
#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_EMSCRIPTEN
#include <emscripten.h>
#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_PNACL
#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/core.h>
#include <ppapi/cpp/module.h>
#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_WINDOWS
#include <Windows.h>
#include <memory>
#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_PORTABLE
#include <algorithm>
#include <condition_variable>
#include <thread>
#include <vector>

#include <stlab/concurrency/task.hpp>
// REVISIT (sparent) : for testing only
#if 0 && __APPLE__
#include <dispatch/dispatch.h>
#endif
#endif

/**************************************************************************************************/

namespace stlab {

/**************************************************************************************************/

inline namespace v1 {
/**************************************************************************************************/

namespace detail {

/**************************************************************************************************/

#if STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_LIBDISPATCH

struct system_timer_type {
    using result_type = void;

    template <typename F>
    void operator()(std::chrono::steady_clock::time_point when, F f) const {
        using namespace std::chrono;

        using f_t = decltype(f);

        dispatch_after_f(
            dispatch_time(0, duration_cast<nanoseconds>(when - steady_clock::now()).count()),
            dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), new f_t(std::move(f)),
            [](void* f_) {
                auto f = static_cast<f_t*>(f_);
                (*f)();
                delete f;
            });
    }
};

#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_EMSCRIPTEN

#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_WINDOWS

class system_timer {
    PTP_POOL _pool = nullptr;
    TP_CALLBACK_ENVIRON _callBackEnvironment;
    PTP_CLEANUP_GROUP _cleanupgroup = nullptr;

public:
    system_timer() {
        InitializeThreadpoolEnvironment(&_callBackEnvironment);
        _pool = CreateThreadpool(nullptr);
        if (_pool == nullptr) throw std::bad_alloc();

        _cleanupgroup = CreateThreadpoolCleanupGroup();
        if (_pool == nullptr) throw std::bad_alloc();

        SetThreadpoolCallbackPool(&_callBackEnvironment, _pool);
        SetThreadpoolCallbackCleanupGroup(&_callBackEnvironment, _cleanupgroup, nullptr);
    }

    ~system_timer() {
        CloseThreadpoolCleanupGroupMembers(_cleanupgroup, FALSE, nullptr);
        CloseThreadpoolCleanupGroup(_cleanupgroup);
        CloseThreadpool(_pool);
    }

    template <typename F>
    void operator()(std::chrono::steady_clock::time_point when, F&& f) {
        auto timer = CreateThreadpoolTimer(&timer_callback_impl<F>, new F(std::forward<F>(f)),
                                           &_callBackEnvironment);

        if (timer == nullptr) {
            throw std::bad_alloc();
        }

        auto file_time = time_point_to_FILETIME(when);

        SetThreadpoolTimer(timer, &file_time, 0, 0);
    }

private:
    template <typename F>
    static void CALLBACK timer_callback_impl(PTP_CALLBACK_INSTANCE /*Instance*/,
                                             PVOID parameter,
                                             PTP_TIMER /*timer*/) {
        std::unique_ptr<F> f(static_cast<F*>(parameter));
        (*f)();
    }

    time_t steady_clock_to_time_t(std::chrono::steady_clock::time_point t) const {
        return std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now() +
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                t - std::chrono::steady_clock::now()));
    }

    FILETIME time_point_to_FILETIME(const std::chrono::steady_clock::time_point& when) const {
        FILETIME ft = {0, 0};
        SYSTEMTIME st = {0};
        time_t t = steady_clock_to_time_t(when);
        tm utc_tm;
        if (!gmtime_s(&utc_tm, &t)) {
            st.wSecond = static_cast<WORD>(utc_tm.tm_sec);
            st.wMinute = static_cast<WORD>(utc_tm.tm_min);
            st.wHour = static_cast<WORD>(utc_tm.tm_hour);
            st.wDay = static_cast<WORD>(utc_tm.tm_mday);
            st.wMonth = static_cast<WORD>(utc_tm.tm_mon + 1);
            st.wYear = static_cast<WORD>(utc_tm.tm_year + 1900);
            st.wMilliseconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(when.time_since_epoch())
                    .count() %
                1000;
            SystemTimeToFileTime(&st, &ft);
        }
        return ft;
    }
};

#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_PORTABLE

class system_timer {
    using element_t = std::pair<std::chrono::steady_clock::time_point, task<void()>>;
    using queue_t = std::vector<element_t>;
    using lock_t = std::unique_lock<std::mutex>;

    queue_t _timed_queue;
    std::condition_variable _condition;
    bool _stop = false;
    std::mutex _timed_queue_mutex;
    std::thread _timed_queue_thread;

    struct greater_first {
        using result_type = bool;

        template <typename T>
        bool operator()(const T& x, const T& y) {
            return x.first > y.first;
        }
    };

    void timed_queue_run() {
        while (true) {
            task<void()> task;
            {
                lock_t lock(_timed_queue_mutex);

                while (_timed_queue.empty() && !_stop)
                    _condition.wait(lock);
                if (_stop) return;
                while (std::chrono::steady_clock::now() < _timed_queue.front().first) {
                    auto when = _timed_queue.front().first;
                    _condition.wait_until(lock, when);
                    if (_stop) return;
                }
                std::pop_heap(begin(_timed_queue), end(_timed_queue), greater_first());
                task = std::move(_timed_queue.back().second);
                _timed_queue.pop_back();
            }

            task();
        }
    }

public:
    system_timer() {
        _timed_queue_thread = std::thread([this] { this->timed_queue_run(); });
    }

    ~system_timer() {
        {
            lock_t lock(_timed_queue_mutex);
            _stop = true;
        }
        _condition.notify_one();
        _timed_queue_thread.join();
    }

    template <typename F>
    void operator()(std::chrono::steady_clock::time_point when, F&& f) {
        lock_t lock(_timed_queue_mutex);
        _timed_queue.emplace_back(when, std::forward<F>(f));
        std::push_heap(std::begin(_timed_queue), std::end(_timed_queue), greater_first());
        _condition.notify_one();
    }
};

#endif

#if (STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_WINDOWS) || \
    (STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_PORTABLE)

struct system_timer_type {
    using result_type = void;

    template <typename F>
    void operator()(std::chrono::steady_clock::time_point when, F&& f) const {
        static system_timer only_system_timer;
        only_system_timer(when, std::forward<F>(f));
    }
};

#endif

/**************************************************************************************************/

} // namespace detail

/**************************************************************************************************/

constexpr auto system_timer = detail::system_timer_type{};

} // namespace v1

/**************************************************************************************************/

} // namespace stlab

/**************************************************************************************************/

#endif

/**************************************************************************************************/
