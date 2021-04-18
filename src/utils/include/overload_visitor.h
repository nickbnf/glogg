/*
 * Copyright (C) 2021 Anton Filimonov
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

namespace {
template <class... Fs>
struct overload;

template <class F0, class... Frest>
struct overload<F0, Frest...> : F0, overload<Frest...> {
    overload( F0 f0, Frest... rest )
        : F0( f0 )
        , overload<Frest...>( rest... )
    {
    }

    using F0::operator();
    using overload<Frest...>::operator();
};

template <class F0>
struct overload<F0> : F0 {
    explicit overload( F0 f0 )
        : F0( f0 )
    {
    }

    using F0::operator();
};
} // namespace

template <class... Fs>
auto make_overload_visitor( Fs... fs )
{
    return overload<Fs...>( fs... );
}
