/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_CONCURRENCY_OPTIONAL_HPP
#define STLAB_CONCURRENCY_OPTIONAL_HPP

#include <stlab/concurrency/config.hpp>

#define STLAB_STD_OPTIONAL 1
#define STLAB_STD_EXPERIMENTAL_OPTIONAL 2
#define STLAB_BOOST_OPTIONAL 3

// The library can be used with boost::optinal, std::experimental::optional or std::optional.
// Without any additional define, it uses the versions from the standard, if it is available.
//
// If using of boost::optional shall be enforced, set the define STLAB_FORCE_BOOST_OPTIONAL

#ifdef STLAB_FORCE_BOOST_OPTIONAL
#include <boost/optional.hpp>
#define STLAB_OPTIONAL STLAB_BOOST_OPTIONAL
#endif

#ifndef STLAB_OPTIONAL
#ifdef __has_include                                     // Check if __has_include is present
#if __has_include(<optional>) && STLAB_CPP_VERSION == 17 // Check for a standard library
#include <optional>
#define STLAB_OPTIONAL STLAB_STD_OPTIONAL
#elif __has_include(<experimental/optional>) // Check for an experimental version
#include <experimental/optional>
#define STLAB_OPTIONAL STLAB_STD_EXPERIMENTAL_OPTIONAL
#endif
#endif
#endif

#ifndef STLAB_OPTIONAL
#include <boost/optional.hpp>
#define STLAB_OPTIONAL STLAB_BOOST_OPTIONAL
#endif

namespace stlab {

#if STLAB_OPTIONAL == STLAB_STD_OPTIONAL

template <typename T>
using optional = std::optional<T>;

constexpr std::nullopt_t nullopt{std::nullopt};

#elif STLAB_OPTIONAL == STLAB_STD_EXPERIMENTAL_OPTIONAL

template <typename T>
using optional = std::experimental::optional<T>;

constexpr std::experimental::nullopt_t nullopt{std::experimental::nullopt};

#elif STLAB_OPTIONAL == STLAB_BOOST_OPTIONAL

template <typename T>
using optional = boost::optional<T>;

const boost::none_t nullopt((boost::none_t::init_tag()));

#else
#error "No optional!"
#endif

} // namespace stlab

#endif