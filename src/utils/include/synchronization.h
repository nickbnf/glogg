/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov
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

#ifndef KLOGG_SYNCHRONIZATION_H
#define KLOGG_SYNCHRONIZATION_H

#include <mutex>

using Mutex = std::mutex;
using ScopedLock = std::scoped_lock<Mutex>;

using RecursiveMutex= std::recursive_mutex;
using ScopedRecursiveLock = std::scoped_lock<RecursiveMutex>;

#endif