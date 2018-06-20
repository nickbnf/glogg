/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_SCOPE_HPP
#define STLAB_SCOPE_HPP

/**************************************************************************************************/

#include <mutex>
#include <tuple>
#include <utility>

/**************************************************************************************************/

namespace stlab {

/**************************************************************************************************/

inline namespace v1 {
/**************************************************************************************************/

namespace detail {

template <typename T, typename Tuple, size_t... S>
auto scope_call(Tuple&& t, std::index_sequence<S...>) {
    T scoped(std::forward<std::tuple_element_t<S, Tuple>>(std::get<S>(t))...);
    (void)scoped;

    // call the function
    constexpr size_t last_index = std::tuple_size<Tuple>::value - 1;
    return std::forward<std::tuple_element_t<last_index, Tuple>>(std::get<last_index>(t))();
}

} // namespace detail

/**************************************************************************************************/

template <typename T, typename... Args>
inline auto scope(Args&&... args) {
    return detail::scope_call<T>(std::forward_as_tuple(std::forward<Args>(args)...),
                                 std::make_index_sequence<sizeof...(args) - 1>());
}

/* Workaround until VS2017 bug is fixed */
template <typename T, typename F>
inline auto scope(std::mutex& m, F&& f) {
    T scoped(m);
    return std::forward<F>(f)();
}

/**************************************************************************************************/

} // namespace v1

/**************************************************************************************************/

} // namespace stlab

/**************************************************************************************************/

#endif

/**************************************************************************************************/
