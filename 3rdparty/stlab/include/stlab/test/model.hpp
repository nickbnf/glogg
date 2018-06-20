/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_MODEL_HPP
#define STLAB_MODEL_HPP

/**************************************************************************************************/

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>

/**************************************************************************************************/

namespace stlab {

/**************************************************************************************************/

inline namespace v1 {
/**************************************************************************************************/

struct annotate_counters {
    std::atomic_size_t _dtor{0};
    std::atomic_size_t _copy_ctor{0};
    std::atomic_size_t _move_ctor{0};
    std::atomic_size_t _copy_assign_lhs{0};
    std::atomic_size_t _copy_assign_rhs{0};
    std::atomic_size_t _move_assign_lhs{0};
    std::atomic_size_t _move_assign_rhs{0};
    std::atomic_size_t _swap{0};
    std::atomic_size_t _equality{0};
    std::mutex _mutex;
    std::condition_variable _condition;

    std::size_t remaining() const { return _copy_ctor + _move_ctor - _dtor + 1; }

    void wait(std::size_t count) {
        std::unique_lock<std::mutex> lock(_mutex);
        while (count != remaining())
            _condition.wait(lock);
    }

    friend inline std::ostream& operator<<(std::ostream& out, const annotate_counters& x) {
        out << "           dtor: " << x._dtor << "\n";
        out << "      copy_ctor: " << x._copy_ctor << "\n";
        out << "      move_ctor: " << x._move_ctor << "\n";
        out << "copy_assign_lhs: " << x._copy_assign_lhs << "\n";
        out << "copy_assign_rhs: " << x._copy_assign_rhs << "\n";
        out << "move_assign_lhs: " << x._move_assign_lhs << "\n";
        out << "move_assign_rhs: " << x._move_assign_rhs << "\n";
        out << "           swap: " << x._swap << "\n";
        out << "       equality: " << x._equality << "\n";

        return out;
    }
};

struct annotate {
    annotate_counters* _counters;
    explicit annotate(annotate_counters& counters) : _counters(&counters) {}

    ~annotate() {
        {
            ++_counters->_dtor;
            _counters->_condition.notify_one();
        }
    }

    annotate(const annotate& x) : _counters(x._counters) { ++_counters->_copy_ctor; }
    annotate(annotate&& x) noexcept : _counters(x._counters) { ++_counters->_move_ctor; }

    annotate& operator=(const annotate& x) {
        ++x._counters->_copy_assign_rhs;
        ++_counters->_copy_assign_lhs;
        return *this;
    }
    annotate& operator=(annotate&& x) noexcept {
        ++x._counters->_move_assign_rhs;
        ++_counters->_move_assign_lhs;
        return *this;
    }

    friend inline void swap(annotate& x, annotate& y) {
        ++x._counters->_swap;
        ++y._counters->_swap;
    }

    friend inline bool operator==(const annotate& x, const annotate& y) {
        ++x._counters->_equality;
        ++y._counters->_equality;
        return true;
    }
    friend inline bool operator!=(const annotate& x, const annotate& y) { return !(x == y); }
};

/**************************************************************************************************/

struct regular {
    int _x;

    explicit regular(int x) : _x(x) { std::cout << _x << " ctor" << std::endl; }
    ~regular() { std::cout << _x << " dtor" << std::endl; }

    regular(const regular& rhs) : _x(rhs._x) { std::cout << _x << " copy-ctor" << std::endl; }
    regular(regular&& rhs) noexcept : _x(std::move(rhs._x)) {
        std::cout << _x << " move-ctor" << std::endl;
        rhs._x = 0;
    }

    regular& operator=(const regular& rhs) {
        std::cout << _x << " assign" << std::endl;
        _x = rhs._x;
        return *this;
    }
    regular& operator=(regular&& rhs) noexcept {
        std::cout << _x << " move-assign" << std::endl;
        _x = std::move(rhs._x);
        rhs._x = 0;
        return *this;
    }

    friend inline void swap(regular& lhs, regular& rhs) {
        std::cout << lhs._x << "/" << rhs._x << " swap " << std::endl;
        std::swap(lhs._x, rhs._x);
    }

    friend inline bool operator==(const regular& lhs, const regular& rhs) {
        return lhs._x == rhs._x;
    }
    friend inline bool operator!=(const regular& lhs, const regular& rhs) { return !(lhs == rhs); }

    friend inline bool operator<(const regular& lhs, const regular& rhs) {
        bool result(lhs._x < rhs._x);
        std::cout << lhs._x << " < " << rhs._x << ": " << std::boolalpha << result << std::endl;
        return result;
    }
};

/**************************************************************************************************/

class move_only {
private:
    int _member{0};

public:
    move_only() {}
    move_only(int member) : _member(member) {}
    move_only(const move_only&) = delete;
    move_only& operator=(const move_only&) = delete;
    move_only(move_only&&) = default;
    move_only& operator=(move_only&&) = default;
    virtual ~move_only() = default;

    int member() { return _member; }
    int member() const { return _member; }
};

/**************************************************************************************************/

} // namespace v1

/**************************************************************************************************/

} // namespace stlab

/**************************************************************************************************/

#endif

/**************************************************************************************************/
