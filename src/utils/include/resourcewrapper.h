/*
 * Copyright (C) 2021 Anton Filimonov and other contributors
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
 */

#ifndef KLOGG_RESOURCE_WRAPPER_H
#define KLOGG_RESOURCE_WRAPPER_H

#include <memory>
#include <type_traits>

template <auto DeleterFunction>
using CustomDeleter = std::integral_constant<decltype( DeleterFunction ), DeleterFunction>;

template <typename ManagedType, auto Functor>
using UniqueResource = std::unique_ptr<ManagedType, CustomDeleter<Functor>>;

template <typename T, auto DeleteFunctor, typename CreateFunc, typename... Args>
UniqueResource<T, DeleteFunctor> makeUniqueResource( CreateFunc createFunc, Args&&... args )
{
    return { createFunc( std::forward<Args>( args )... ), {} };
}

template <typename ManagedType>
using SharedResource = std::shared_ptr<ManagedType>;

#endif
