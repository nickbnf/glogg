/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_CONCURRENCY_EXECUTOR_BASE_HPP
#define STLAB_CONCURRENCY_EXECUTOR_BASE_HPP

#include <chrono>
#include <functional>

#include <stlab/concurrency/system_timer.hpp>
#include <stlab/concurrency/task.hpp>

/**************************************************************************************************/

namespace stlab {

/**************************************************************************************************/

inline namespace v1 {
/**************************************************************************************************/

using executor_t = std::function<void(stlab::task<void()>)>;

/*
 * returns an executor that will schedule any passed task to it to execute
 * at the given time point on the executor provided
 */

inline executor_t execute_at(std::chrono::steady_clock::time_point when, executor_t executor) {
    return [_when = std::move(when), _executor = std::move(executor)](auto f) mutable {
        if ((_when != std::chrono::steady_clock::time_point()) &&
            (_when > std::chrono::steady_clock::now()))
            system_timer(_when, [_f = std::move(f), _executor = std::move(_executor)]() mutable {
                _executor(std::move(_f));
            });
        else
            _executor(std::move(f));
    };
}

/*
 * returns an executor that will schedule the task to execute on the provided
 * executor duration after it is invoked
 */

template <typename E>
auto execute_delayed(std::chrono::steady_clock::duration duration, E executor) {
    return execute_at(std::chrono::steady_clock::now() + duration, std::move(executor));
}

/**************************************************************************************************/

} // namespace v1

/**************************************************************************************************/

} // namespace stlab

/**************************************************************************************************/

#endif

/**************************************************************************************************/
