/*
    Copyright 2013 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
/**************************************************************************************************/

#ifndef STLAB_COPY_ON_WRITE_HPP
#define STLAB_COPY_ON_WRITE_HPP

/**************************************************************************************************/

#include <atomic>
#include <cassert>
#include <cstddef>
#include <utility>

/**************************************************************************************************/

namespace stlab {

/**************************************************************************************************/

template <typename T> // T models Regular
class copy_on_write {
    struct model {
        std::atomic<std::size_t> _count{1};

        model() = default;

        template <class... Args>
        explicit model(Args&&... args) : _value(std::forward<Args>(args)...) {}

        T _value;
    };

    model* _self;

    template <class U>
    using disable_copy = std::enable_if_t<!std::is_same<std::decay_t<U>, copy_on_write>::value>*;

    template <typename U>
    using disable_copy_assign =
        std::enable_if_t<!std::is_same<std::decay_t<U>, copy_on_write>::value, copy_on_write&>;

public:
    /* [[deprecated]] */ using value_type = T;

    using element_type = T;

    copy_on_write() {
        static model default_s;
        _self = &default_s;

        // coverity[useless_call]
        ++_self->_count;
    }

    template <class U>
    copy_on_write(U&& x, disable_copy<U> = nullptr) : _self(new model(std::forward<U>(x))) {}

    template <class U, class V, class... Args>
    copy_on_write(U&& x, V&& y, Args&&... args)
        : _self(new model(std::forward<U>(x), std::forward<V>(y), std::forward<Args>(args)...)) {}

    copy_on_write(const copy_on_write& x) noexcept : _self(x._self) {
        assert(_self && "FATAL (sparent) : using a moved copy_on_write object");

        // coverity[useless_call]
        ++_self->_count;
    }
    copy_on_write(copy_on_write&& x) noexcept : _self(x._self) {
        assert(_self && "WARNING (sparent) : using a moved copy_on_write object");
        x._self = nullptr;
    }

    ~copy_on_write() {
        if (_self && (--_self->_count == 0)) delete _self;
    }

    auto operator=(const copy_on_write& x) noexcept -> copy_on_write& {
        return *this = copy_on_write(x);
    }

    auto operator=(copy_on_write&& x) noexcept -> copy_on_write& {
        auto tmp = std::move(x);
        swap(*this, tmp);
        return *this;
    }

    template <class U>
    auto operator=(U&& x) -> disable_copy_assign<U> {
        if (_self && unique()) {
            _self->_value = std::forward<U>(x);
            return *this;
        }

        return *this = copy_on_write(std::forward<U>(x));
    }

    auto write() -> element_type& {
        if (!unique()) *this = copy_on_write(read());

        return _self->_value;
    }

    auto read() const noexcept -> const element_type& {
        assert(_self && "FATAL (sparent) : using a moved copy_on_write object");

        return _self->_value;
    }

    operator const element_type&() const noexcept { return read(); }

    auto operator*() const noexcept -> const element_type& { return read(); }

    auto operator-> () const noexcept -> const element_type* { return &read(); }

    bool unique() const noexcept {
        assert(_self && "FATAL (sparent) : using a moved copy_on_write object");

        return _self->_count == 1;
    }
    [[deprecated]] bool unique_instance() const noexcept { return unique(); }

    bool identity(const copy_on_write& x) const noexcept {
        assert((_self && x._self) && "FATAL (sparent) : using a moved copy_on_write object");

        return _self == x._self;
    }

    friend inline void swap(copy_on_write& x, copy_on_write& y) noexcept {
        std::swap(x._self, y._self);
    }

    friend inline bool operator<(const copy_on_write& x, const copy_on_write& y) noexcept {
        return *x < *y;
    }

    friend inline bool operator<(const copy_on_write& x, const element_type& y) noexcept {
        return *x < y;
    }

    friend inline bool operator<(const element_type& x, const copy_on_write& y) noexcept {
        return x < *y;
    }

    friend inline bool operator>(const copy_on_write& x, const copy_on_write& y) noexcept {
        return y < x;
    }

    friend inline bool operator>(const copy_on_write& x, const element_type& y) noexcept {
        return y < x;
    }

    friend inline bool operator>(const element_type& x, const copy_on_write& y) noexcept {
        return y < x;
    }

    friend inline bool operator<=(const copy_on_write& x, const copy_on_write& y) noexcept {
        return !(y < x);
    }

    friend inline bool operator<=(const copy_on_write& x, const element_type& y) noexcept {
        return !(y < x);
    }

    friend inline bool operator<=(const element_type& x, const copy_on_write& y) noexcept {
        return !(y < x);
    }

    friend inline bool operator>=(const copy_on_write& x, const copy_on_write& y) noexcept {
        return !(x < y);
    }

    friend inline bool operator>=(const copy_on_write& x, const element_type& y) noexcept {
        return !(x < y);
    }

    friend inline bool operator>=(const element_type& x, const copy_on_write& y) noexcept {
        return !(x < y);
    }

    friend inline bool operator==(const copy_on_write& x, const copy_on_write& y) noexcept {
        return *x == *y;
    }

    friend inline bool operator==(const copy_on_write& x, const element_type& y) noexcept {
        return *x == y;
    }

    friend inline bool operator==(const element_type& x, const copy_on_write& y) noexcept {
        return x == *y;
    }

    friend inline bool operator!=(const copy_on_write& x, const copy_on_write& y) noexcept {
        return !(x == y);
    }

    friend inline bool operator!=(const copy_on_write& x, const element_type& y) noexcept {
        return !(x == y);
    }

    friend inline bool operator!=(const element_type& x, const copy_on_write& y) noexcept {
        return !(x == y);
    }
};
/**************************************************************************************************/

} // namespace stlab

/**************************************************************************************************/

#endif

/**************************************************************************************************/
