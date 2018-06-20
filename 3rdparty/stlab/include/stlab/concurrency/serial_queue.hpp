/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_CONCURRENCY_SERIAL_QUEUE_HPP
#define STLAB_CONCURRENCY_SERIAL_QUEUE_HPP

/**************************************************************************************************/

#include <deque>
#include <mutex>
#include <tuple>
#include <utility>

#include <stlab/scope.hpp>

#define STLAB_DISABLE_FUTURE_COROUTINES

#include <stlab/concurrency/future.hpp>
#include <stlab/concurrency/task.hpp>

/**************************************************************************************************/

namespace stlab {

/**************************************************************************************************/

inline namespace v1 {
/**************************************************************************************************/

enum class schedule_mode { single, all };

/**************************************************************************************************/

namespace detail {

/**************************************************************************************************/

class serial_instance_t : public std::enable_shared_from_this<serial_instance_t> {
    using executor_t = std::function<void(task<void()>&&)>;
    using queue_t = std::deque<task<void()>>;
    using lock_t = std::lock_guard<std::mutex>;

    std::mutex _m;
    bool _running{false}; // mutex protects this
    queue_t _queue;       // mutex protects this
    executor_t _executor;
    void (serial_instance_t::*_kickstart)();

    static auto pop_front_unsafe(queue_t& q) {
        auto f = std::move(q.front());
        q.pop_front();
        return f;
    }

    bool empty() {
        bool empty;

        scope<lock_t>(_m, [&]() {
            empty = _queue.empty();

            if (empty) {
                _running = false;
            }
        });

        return empty;
    }

    void all() {
        queue_t local_queue;

        scope<lock_t>(_m, [&]() { std::swap(local_queue, _queue); });

        while (!local_queue.empty()) {
            pop_front_unsafe(local_queue)();
        }

        if (!empty()) _executor([_this(shared_from_this())]() { _this->all(); });
    }

    void single() {
        task<void()> f;

        scope<lock_t>(_m, [&]() { f = pop_front_unsafe(_queue); });

        f();

        if (!empty()) _executor([_this(shared_from_this())]() { _this->single(); });
    }

    // The kickstart allows us to grab a pointer to either the single or all
    // routine at construction time. When it comes time to process the queue, we
    // call either via the abstracted _kickstart.
    void kickstart() { (this->*_kickstart)(); }

    static auto kickstarter(schedule_mode mode) {
        switch (mode) {
            case schedule_mode::single:
                return &serial_instance_t::single;
            case schedule_mode::all:
                return &serial_instance_t::all;
        }

        // silence some compilers...
        return &serial_instance_t::single;
    }

public:
    template <typename F>
    void enqueue(F&& f) {
        bool running(true);

        scope<lock_t>(_m, [&]() {
            _queue.emplace_back(std::forward<F>(f));

            // A trick to get the value of _running within the lock scope, but then
            // use it outside the scope, after the lock has been released. It also
            // sets running to true if it is not yet; two birds, one stone.
            std::swap(running, _running);
        });

        if (!running) {
            _executor([_this(shared_from_this())]() { _this->kickstart(); });
        }
    }

    serial_instance_t(executor_t executor, schedule_mode mode) :
        _executor(std::move(executor)), _kickstart(kickstarter(mode)) {}
};

/**************************************************************************************************/

} // namespace detail

/**************************************************************************************************/

class serial_queue_t {
    std::shared_ptr<detail::serial_instance_t> _impl;

public:
    template <typename Executor>
    explicit serial_queue_t(Executor e, schedule_mode mode = schedule_mode::single) :
        _impl(std::make_shared<detail::serial_instance_t>(
            [_e = std::move(e)](auto&& f) { _e(std::forward<decltype(f)>(f)); }, mode)) {}

    auto executor() const {
        return [_impl = _impl](auto&& f) { _impl->enqueue(std::forward<decltype(f)>(f)); };
    }

    template <typename F, typename... Args>
    auto operator()(F&& f, Args&&... args) {
        return async([&_impl = _impl](auto&& f) { _impl->enqueue(std::forward<decltype(f)>(f)); },
                     std::forward<F>(f), std::forward<Args>(args)...);
    }
};

/**************************************************************************************************/

} // namespace v1

/**************************************************************************************************/

} // namespace stlab

/**************************************************************************************************/

#endif

/**************************************************************************************************/
