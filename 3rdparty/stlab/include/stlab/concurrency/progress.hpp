/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_CONCURRENCY_PROGRESS_HPP
#define STLAB_CONCURRENCY_PROGRESS_HPP

#include <atomic>
#include <functional>
#include <memory>

namespace stlab {
namespace detail {
class tracker_server {
    std::atomic_size_t _task_number = {0};
    std::atomic_size_t _done_tasks = {0};
    std::function<void(size_t, size_t)> _signal;

public:
    tracker_server() = default;
    tracker_server(std::function<void(size_t, size_t)> f) : _signal(std::move(f)) {}

    void task_done() {
        ++_done_tasks;
        if (_signal) {
            _signal(steps(), completed());
        }
    }

    void drop_task() { --_task_number; }

    void add_task() { ++_task_number; }

    size_t steps() const { return _task_number.load(); }

    size_t completed() const { return _done_tasks.load(); }
};

template <typename F>
struct tracker_client {
    F _f;
    mutable std::weak_ptr<tracker_server> _p;

    tracker_client() = default;
    tracker_client(const tracker_client& x) : _f(x._f), _p(x._p) {
        auto p = _p.lock();
        if (p) p->add_task();
    }

    tracker_client& operator=(const tracker_client& x) {
        _f = x._f;
        _p = x._p;
        auto p = _p.lock();
        if (p) p->add_task();
        return *this;
    }

    tracker_client(tracker_client&&) = default;
    tracker_client& operator=(tracker_client&&) = default;

    tracker_client(std::shared_ptr<tracker_server>& p, F&& f) : _f(std::move(f)), _p(p) {
        p->add_task();
    }

    ~tracker_client() {
        auto p = _p.lock();
        if (p) p->drop_task();
    }

    template <typename... Args>
    auto operator()(Args&&... args) const {
        auto r = _f(args...);
        auto p = _p.lock();
        if (p) p->task_done();
        return r;
    }
};
} // namespace detail

class progress_tracker {
    std::shared_ptr<detail::tracker_server> _tracker;

public:
    progress_tracker() : _tracker(std::make_shared<detail::tracker_server>()) {}

    progress_tracker(std::function<void(size_t, size_t)> f) :
        _tracker(std::make_shared<detail::tracker_server>(std::move(f))) {}
    progress_tracker(const progress_tracker&) = default;
    progress_tracker& operator=(const progress_tracker&) = default;
    progress_tracker(progress_tracker&&) = default;
    progress_tracker& operator=(progress_tracker&&) = default;

    template <typename F>
    auto operator()(F&& f) {
        return detail::tracker_client<F>(_tracker, std::forward<F>(f));
    }

    size_t steps() const { return _tracker->steps(); }

    size_t completed() const { return _tracker->completed(); }
};
} // namespace stlab

#endif
