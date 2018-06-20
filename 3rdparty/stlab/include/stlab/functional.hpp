/*
    Copyright 2017 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_FUNCTIONAL_HPP
#define STLAB_FUNCTIONAL_HPP

/**************************************************************************************************/

#include <functional>
#include <type_traits>

/**************************************************************************************************/

namespace stlab {

/**************************************************************************************************/

inline namespace v1 {
/**************************************************************************************************/

template <class T>
struct unwrap_reference {
    using type = T;
};

template <class T>
struct unwrap_reference<std::reference_wrapper<T>> {
    using type = T;
};

template <class T>
using unwrap_reference_t = typename unwrap_reference<T>::type;

/**************************************************************************************************/

template <class T>
struct is_reference_wrapper : std::false_type {};
template <class T>
struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type {};

template <class T>
constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;

/**************************************************************************************************/

} // namespace v1

/**************************************************************************************************/

} // namespace stlab

/**************************************************************************************************/

#endif

/**************************************************************************************************/
