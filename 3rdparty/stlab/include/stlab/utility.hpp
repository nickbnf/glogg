/*
    Copyright 2017 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_UTILITY_HPP
#define STLAB_UTILITY_HPP

/**************************************************************************************************/

#include <array>
#include <type_traits>

/**************************************************************************************************/

namespace stlab {

/**************************************************************************************************/

inline namespace v1 {
/**************************************************************************************************/

namespace detail {

/**************************************************************************************************/

template <bool, class T>
struct move_if_helper;

template <class T>
struct move_if_helper<true, T> {
    using type = std::remove_reference_t<T>&&;
};

template <class T>
struct move_if_helper<false, T> {
    using type = std::remove_reference_t<T>&;
};

template <bool P, class T>
using move_if_helper_t = typename move_if_helper<P, T>::type;

/**************************************************************************************************/

} // namespace detail

/**************************************************************************************************/

template <class Seq1, class Seq2>
struct index_sequence_cat;

template <std::size_t... N1, std::size_t... N2>
struct index_sequence_cat<std::index_sequence<N1...>, std::index_sequence<N2...>> {
    using type = std::index_sequence<N1..., N2...>;
};

template <class Seq1, class Seq2>
using index_sequence_cat_t = typename index_sequence_cat<Seq1, Seq2>::type;

/**************************************************************************************************/

template <class Seq>
struct index_sequence_to_array;

template <std::size_t... N>
struct index_sequence_to_array<std::index_sequence<N...>> {
    static constexpr std::array<std::size_t, sizeof...(N)> value{{N...}};
};

/**************************************************************************************************/

template <class Seq, template <std::size_t> class F, std::size_t Index, std::size_t Count>
struct index_sequence_transform;

template <class Seq,
          template <std::size_t> class F,
          std::size_t Index = 0,
          std::size_t Count = Seq::size()>
using index_sequence_transform_t = typename index_sequence_transform<Seq, F, Index, Count>::type;

template <class Seq, template <std::size_t> class F, std::size_t Index, std::size_t Count>
struct index_sequence_transform {
    using type = index_sequence_cat_t<
        index_sequence_transform_t<Seq, F, Index, Count / 2>,
        index_sequence_transform_t<Seq, F, Index + Count / 2, Count - Count / 2>>;
};

template <class Seq, template <std::size_t> class F, std::size_t Index>
struct index_sequence_transform<Seq, F, Index, 0> {
    using type = std::index_sequence<>;
};

template <class Seq, template <std::size_t> class F, std::size_t Index>
struct index_sequence_transform<Seq, F, Index, 1> {
    using type = typename F<index_sequence_to_array<Seq>::value[Index]>::type;
};

/**************************************************************************************************/

template <bool P, class T>
constexpr detail::move_if_helper_t<P, T> move_if(T&& t) noexcept {
    return static_cast<detail::move_if_helper_t<P, T>>(t);
}

/**************************************************************************************************/

template <class F, class... Args>
void for_each_argument(F&& f, Args&&... args) {
    return (void)std::initializer_list<int>{(std::forward<F>(f)(args), 0)...};
}

/**************************************************************************************************/

} // namespace v1

/**************************************************************************************************/

} // namespace stlab

/**************************************************************************************************/

#endif

/**************************************************************************************************/
