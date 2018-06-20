/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_CONCURRENCY_DEFAULT_EXECUTOR_HPP
#define STLAB_CONCURRENCY_DEFAULT_EXECUTOR_HPP

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
#include <atomic>
#include <condition_variable>
#include <deque>
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

struct default_executor_type {
private:
    struct group {
        dispatch_group_t _group = dispatch_group_create();
        ~group() {
            dispatch_group_wait(_group, DISPATCH_TIME_FOREVER);
            dispatch_release(_group);
        }
    };

public:
    using result_type = void;
    template <typename F>
    void operator()(F f) const {
        using f_t = decltype(f);

        static group g;

        dispatch_group_async_f(g._group,
                               dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
                               new f_t(std::move(f)), [](void* f_) {
                                   auto f = static_cast<f_t*>(f_);
                                   (*f)();
                                   delete f;
                               });
    }
};

#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_EMSCRIPTEN

struct default_executor_type {
    using result_type = void;

    template <typename F>
    void operator()(F f) const {
        // REVISIT (sparent) : Using a negative timeout may give better performance. Need to test.
        using f_t = decltype(f);

        emscripten_async_call(
            [](void* f_) {
                auto f = static_cast<f_t*>(f_);
                (*f)();
                delete f;
            },
            new f_t(std::move(f)), 0);
    }
};

#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_PNACL

struct default_executor_type {
    using result_type = void;

    template <typename F>
    void operator()(F f) const {
        using f_t = decltype(f);

        pp::Module::Get()->core()->CallOnMainThread(0,
                                                    pp::CompletionCallback(
                                                        [](void* f_, int32_t) {
                                                            auto f = static_cast<f_t*>(f_);
                                                            (*f)();
                                                            delete f;
                                                        },
                                                        new f_t(std::move(f))),
                                                    0);
    }
};

#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_WINDOWS

class task_system {
    PTP_POOL _pool = nullptr;
    TP_CALLBACK_ENVIRON _callBackEnvironment;
    PTP_CLEANUP_GROUP _cleanupgroup = nullptr;

public:
    task_system() {
        InitializeThreadpoolEnvironment(&_callBackEnvironment);
        _pool = CreateThreadpool(nullptr);
        if (_pool == nullptr) throw std::bad_alloc();

        _cleanupgroup = CreateThreadpoolCleanupGroup();
        if (_pool == nullptr) throw std::bad_alloc();

        SetThreadpoolCallbackPool(&_callBackEnvironment, _pool);
        SetThreadpoolCallbackCleanupGroup(&_callBackEnvironment, _cleanupgroup, nullptr);
    }

    ~task_system() {
        CloseThreadpoolCleanupGroupMembers(_cleanupgroup, FALSE, nullptr);
        CloseThreadpoolCleanupGroup(_cleanupgroup);
        CloseThreadpool(_pool);
    }

    template <typename F>
    void operator()(F&& f) {
        auto work = CreateThreadpoolWork(&callback_impl<F>, new F(std::forward<F>(f)),
                                         &_callBackEnvironment);

        if (work == nullptr) {
            throw std::bad_alloc();
        }
        SubmitThreadpoolWork(work);
    }

private:
    template <typename F>
    static void CALLBACK callback_impl(PTP_CALLBACK_INSTANCE /*instance*/,
                                       PVOID parameter,
                                       PTP_WORK /*Work*/) {
        std::unique_ptr<F> f(static_cast<F*>(parameter));
        (*f)();
    }
};

#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_PORTABLE

/**************************************************************************************************/

class notification_queue {
    using lock_t = std::unique_lock<std::mutex>;
    std::deque<task<void()>> _q;
    bool _done{false};
    std::mutex _mutex;
    std::condition_variable _ready;

public:
    bool try_pop(task<void()>& x) {
        lock_t lock{_mutex, std::try_to_lock};
        if (!lock || _q.empty()) return false;
        x = std::move(_q.front());
        _q.pop_front();
        return true;
    }

    template <typename F>
    bool try_push(F&& f) {
        {
            lock_t lock{_mutex, std::try_to_lock};
            if (!lock) return false;
            _q.emplace_back(std::forward<F>(f));
        }
        _ready.notify_one();
        return true;
    }

    void done() {
        {
            lock_t lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool pop(task<void()>& x) {
        lock_t lock{_mutex};
        while (_q.empty() && !_done)
            _ready.wait(lock);
        if (_q.empty()) return false;
        x = std::move(_q.front());
        _q.pop_front();
        return true;
    }

    template <typename F>
    void push(F&& f) {
        {
            lock_t lock{_mutex};
            _q.emplace_back(std::forward<F>(f));
        }
        _ready.notify_one();
    }
};

/**************************************************************************************************/

class task_system {
    const unsigned _count{std::thread::hardware_concurrency()};
    std::vector<std::thread> _threads;
    std::vector<notification_queue> _q{_count};
    std::atomic<unsigned> _index{0};

    void run(unsigned i) {
        while (true) {
            task<void()> f;

            for (unsigned n = 0; n != _count * 32; ++n) {
                if (_q[(i + n) % _count].try_pop(f)) break;
            }
            if (!f && !_q[i].pop(f)) break;

            f();
        }
    }

public:
    task_system() {
        for (unsigned n = 0; n != _count; ++n) {
            _threads.emplace_back([&, n] { run(n); });
        }
    }

    ~task_system() {
        for (auto& e : _q)
            e.done();

        for (auto& e : _threads)
            e.join();
    }

    template <typename F>
    void operator()(F&& f) {
        auto i = _index++;

        for (unsigned n = 0; n != _count; ++n) {
            if (_q[(i + n) % _count].try_push(std::forward<F>(f))) return;
        }

        _q[i % _count].push(std::forward<F>(f));
    }
};

#endif

#if (STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_PORTABLE) || \
    (STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_WINDOWS)

struct default_executor_type {
    using result_type = void;

    template <typename F>
    void operator()(F&& f) const {
        static task_system only_task_system;
        only_task_system(std::forward<F>(f));
    }
};

#endif
/**************************************************************************************************/

} // namespace detail

/**************************************************************************************************/

constexpr auto default_executor = detail::default_executor_type{};

/**************************************************************************************************/

} // namespace v1

/**************************************************************************************************/

} // namespace stlab

/**************************************************************************************************/

#endif // STLAB_CONCURRENCY_DEFAULT_EXECUTOR_HPP
