/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_CONCURRENCY_MAIN_EXECUTOR_HPP
#define STLAB_CONCURRENCY_MAIN_EXECUTOR_HPP

#include "config.hpp"

#define STLAB_MAIN_EXECUTOR_LIBDISPATCH 1
#define STLAB_MAIN_EXECUTOR_EMSCRIPTEN 2
#define STLAB_MAIN_EXECUTOR_PNACL 3
#define STLAB_MAIN_EXECUTOR_WINDOWS 4
#define STLAB_MAIN_EXECUTOR_QT 5
#define STLAB_MAIN_EXECUTOR_PORTABLE 6


#if defined(QT_CORE_LIB) && !defined(STLAB_DISABLE_QT_MAIN_EXECUTOR)
#define STLAB_MAIN_EXECUTOR STLAB_MAIN_EXECUTOR_QT
#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_LIBDISPATCH
#define STLAB_MAIN_EXECUTOR STLAB_MAIN_EXECUTOR_LIBDISPATCH
#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_EMSCRIPTEN
#define STLAB_MAIN_EXECUTOR STLAB_MAIN_EXECUTOR_EMSCRIPTEN
#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_PNACL
#define STLAB_MAIN_EXECUTOR STLAB_MAIN_EXECUTOR_PNACL
#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_WINDOWS
#define STLAB_MAIN_EXECUTOR STLAB_MAIN_EXECUTOR_WINDOWS
#elif STLAB_TASK_SYSTEM == STLAB_TASK_SYSTEM_PORTABLE
#define STLAB_MAIN_EXECUTOR STLAB_MAIN_EXECUTOR_PORTABLE
#endif

#if STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_QT
#include <QApplication>
#include <QEvent>
#include <stlab/concurrency/task.hpp>
#include <memory>
#elif STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_LIBDISPATCH
#include <dispatch/dispatch.h>
#elif STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_EMSCRIPTEN
#include <stlab/concurrency/default_executor.hpp>
#elif STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_PNACL
#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/core.h>
#include <ppapi/cpp/module.h>
#elif STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_WINDOWS
#include <Windows.h>
#elif STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_PORTABLE
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

#if STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_QT

class main_executor_type {
    using result_type = void;

    struct event_receiver;

    class executor_event : public QEvent {
        stlab::task<void()> _f;
        std::unique_ptr<event_receiver> _receiver;

    public:
        executor_event() : QEvent(QEvent::User), _receiver(new event_receiver()) {
            _receiver->moveToThread(QApplication::instance()->thread());
        }

        template <typename F>
        void set_task(F&& f) {
            _f = std::forward<F>(f);
        }

        void execute() { _f(); }

        QObject* receiver() const { return _receiver.get(); }
    };

    struct event_receiver : public QObject {
        bool event(QEvent* event) override {
            auto myEvent = dynamic_cast<executor_event*>(event);
            if (myEvent) {
                myEvent->execute();
                return true;
            }
            return false;
        }
    };

public:
    template <typename F>
    void operator()(F f) const {
        auto event = std::make_unique<executor_event>();
        event->set_task(std::move(f));
        QApplication::postEvent(event->receiver(), event.release());
    }
};

/**************************************************************************************************/

#elif STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_LIBDISPATCH

struct main_executor_type {
    using result_type = void;

    template <typename F>
    void operator()(F f) {
        using f_t = decltype(f);

        dispatch_async_f(dispatch_get_main_queue(), new f_t(std::move(f)), [](void* f_) {
            auto f = static_cast<f_t*>(f_);
            (*f)();
            delete f;
        });
    }
};

#elif STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_EMSCRIPTEN

using main_executor_type = default_executor_type;

#elif (STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_PORTABLE) || \
    (STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_PNACL) ||  \
    (STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_WINDOWS)

// TODO (sparent) : We need a main task scheduler for STLAB_TASK_SYSTEM_WINDOWS

#if STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_PNACL

struct main_executor_type {
    using result_type = void;

    template <typename F>
    void operator()(F f) {
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

#elif STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_WINDOWS

// TODO main_executor_type for Windows 8 / 10
struct main_executor_type {};

#elif STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_PORTABLE

// TODO (sparent) : provide a scheduler and run-loop - this is provide for testing on mac
struct main_executor_type {
    using result_type = void;

#if __APPLE__
    template <typename F>
    void operator()(F f) {
        using f_t = decltype(f);

        ::dispatch_async_f(dispatch_get_main_queue(), new f_t(std::move(f)), [](void* f_) {
            auto f = static_cast<f_t*>(f_);
            (*f)();
            delete f;
        });
    }
#endif // __APPLE__
};

#endif // STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_PNACL

#endif // (STLAB_MAIN_EXECUTOR == STLAB_MAIN_EXECUTOR_PORTABLE) || ...

/**************************************************************************************************/

} // namespace detail

/**************************************************************************************************/

constexpr auto main_executor = detail::main_executor_type{};

/**************************************************************************************************/

} // namespace v1

/**************************************************************************************************/

} // namespace stlab

/**************************************************************************************************/

#endif

/**************************************************************************************************/
