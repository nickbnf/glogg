/*
 * Copyright (C) 2020 Anton Filimonov and other contributors
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

#include "memory_info.h"

#include <QtGlobal>

#include <mutex>

#if defined( Q_OS_WIN )

#include <windows.h>

uint64_t systemPhysicalMemory()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof( status );
    GlobalMemoryStatusEx( &status );
    return status.ullTotalPhys;
}

#elif defined( Q_OS_APPLE )

#include <sys/types.h>
#include <sys/sysctl.h>

uint64_t systemPhysicalMemory()
{
    uint64_t memory;
    size_t size = sizeof( memory );
    sysctlbyname( "hw.memsize", &memory, &size, NULL, 0 );
    return memory;
}

#else

#include <sys/types.h>
#include <unistd.h>

uint64_t systemPhysicalMemory()
{
    const auto pageSize = static_cast<uint64_t>( sysconf( _SC_PAGE_SIZE ) );
    const auto pages = static_cast<uint64_t>( sysconf( _SC_PHYS_PAGES ) );
    return pages * pageSize;
}

#endif

namespace {
std::once_flag totalMemoryFlag;
static uint64_t totalMemory{};
} // namespace

uint64_t physicalMemory()
{
    std::call_once( totalMemoryFlag, []() { totalMemory = systemPhysicalMemory(); } );
    return totalMemory;
}
