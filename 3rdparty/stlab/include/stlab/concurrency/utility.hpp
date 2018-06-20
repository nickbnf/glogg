/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_CONCURRENCY_UTILITY_HPP
#define STLAB_CONCURRENCY_UTILITY_HPP

#include <condition_variable>
#include <exception>
#include <mutex>
#include <stlab/concurrency/future.hpp>
#include <stlab/concurrency/immediate_executor.hpp>
#include <stlab/concurrency/optional.hpp>
#include <type_traits>

#include <stlab/memory.hpp>

/**************************************************************************************************/

#if 0

#include <thread>

// usefull makro for debugging
#define STLAB_TRACE(S)                          \
    printf("%s:%d %d %s\n", __FILE__, __LINE__, \
           (int)std::hash<std::thread::id>()(std::this_thread::get_id()), S);

#endif

/**************************************************************************************************/

namespace stlab {

/**************************************************************************************************/

inline namespace v1 {
namespace detail {

struct shared_state {
    bool flag{false};
    std::condition_variable condition;
    std::mutex m;
    std::exception_ptr error{nullptr};
};

template <typename T>
struct shared_state_result : shared_state {
    stlab::optional<T> result;
};
} // namespace detail

/**************************************************************************************************/

template <typename T, typename E>
future<std::decay_t<T>> make_ready_future(T&& x, E executor) {
    auto p = package<std::decay_t<T>(std::decay_t<T>)>(
        std::move(executor), [](auto&& x) { return std::forward<decltype(x)>(x); });
    p.first(std::forward<T>(x));
    return p.second;
}

template <typename E>
future<void> make_ready_future(E executor) {
    auto p = package<void()>(std::move(executor), []() {});
    p.first();
    return p.second;
}

template <typename T, typename E>
future<T> make_exceptional_future(std::exception_ptr error, E executor) {
    auto p = package<T(T)>(std::move(executor), [_error = error](auto&& x) {
        std::rethrow_exception(_error);
        return std::forward<decltype(x)>(x);
    });
    p.first(T{});
    return p.second;
}

template <typename T>
T blocking_get(future<T> x) {
    stlab::optional<T> result;
    std::exception_ptr error = nullptr;

    bool flag{false};
    std::condition_variable condition;
    std::mutex m;
    auto hold = std::move(x).recover(immediate_executor, [&](auto&& r) {
        if (r.error())
            error = *std::forward<decltype(r)>(r).error();
        else
            result = std::move(*std::forward<decltype(r)>(r).get_try());

        {
            std::unique_lock<std::mutex> lock{m};
            flag = true;
            condition.notify_one();
        }
    });
    {
        std::unique_lock<std::mutex> lock{m};
        while (!flag) {
            condition.wait(lock);
        }
    }

    if (error) std::rethrow_exception(error);

    return std::move(*result);
}

template <typename T>
stlab::optional<T> blocking_get(future<T> x, const std::chrono::nanoseconds& timeout) {
    std::exception_ptr error = nullptr;
    auto state = std::make_shared<detail::shared_state_result<T>>();
    auto hold =
        std::move(x).recover(immediate_executor, [_weak_state = make_weak_ptr(state)](auto&& r) {
            auto state = _weak_state.lock();
            if (!state) {
                return;
            }
            if (r.error())
                state->error = *std::forward<decltype(r)>(r).error();
            else
                state->result = std::move(*std::forward<decltype(r)>(r).get_try());
            {
                std::unique_lock<std::mutex> lock{state->m};
                state->flag = true;
            }
            state->condition.notify_one();
        });

    {
        std::unique_lock<std::mutex> lock{state->m};
        while (!state->flag) {
            if (state->condition.wait_for(lock, timeout) == std::cv_status::timeout) {
                hold.reset();
                return stlab::optional<T>{};
            }
        }
    }

    if (state->error) std::rethrow_exception(state->error);

    return std::move(state->result);
}

inline void blocking_get(future<void> x) {
    std::exception_ptr error = nullptr;

    bool flag{false};
    std::condition_variable condition;
    std::mutex m;
    auto hold = std::move(x).recover(immediate_executor, [&](auto&& r) {
        if (r.error()) error = *std::forward<decltype(r)>(r).error();
        {
            std::unique_lock<std::mutex> lock(m);
            flag = true;
            condition.notify_one();
        }
    });
    {
        std::unique_lock<std::mutex> lock(m);
        while (!flag) {
            condition.wait(lock);
        }
    }

    if (error) std::rethrow_exception(error);
}

inline bool blocking_get(future<void> x, const std::chrono::nanoseconds& timeout) {
    auto state = std::make_shared<detail::shared_state>();
    auto hold =
        std::move(x).recover(immediate_executor, [_weak_state = make_weak_ptr(state)](auto&& r) {
            auto state = _weak_state.lock();
            if (!state) {
                return;
            }
            if (r.error()) state->error = *std::forward<decltype(r)>(r).error();
            {
                std::unique_lock<std::mutex> lock(state->m);
                state->flag = true;
            }
            state->condition.notify_one();
        });

    {
        std::unique_lock<std::mutex> lock(state->m);
        while (!state->flag) {
            if (state->condition.wait_for(lock, timeout) == std::cv_status::timeout) {
                hold.reset();
                return false;
            }
        }
    }

    if (state->error) std::rethrow_exception(state->error);
    return true;
}

/**************************************************************************************************/

} // namespace v1

/**************************************************************************************************/

} // namespace stlab

#endif // SLABFUTURE_UTILITY_HPP
