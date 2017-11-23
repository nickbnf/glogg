/*
 * Copyright (C) 2015 Nicolas Bonnefon and other contributors
 *
 * This file is part of glogg.
 *
 * glogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * glogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with glogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef THREADPRIVATESTORE_H
#define THREADPRIVATESTORE_H

#include <QAtomicInt>
#include <QThread>
#include <cassert>

#include "log.h"

// This template stores data that is private to the calling thread
// in a completely wait-free manner.
template <typename T, int MAX_THREADS>
class ThreadPrivateStore
{
  public:
    // Construct an empty ThreadPrivateStore
    ThreadPrivateStore() : nb_threads_( 0 ) {
        // Last one is a guard
        for ( int i=0; i<(MAX_THREADS+1); ++i )
            thread_ids_[i] = 0;
    }

    // Conversion to a T
    operator T() const
    { return get(); }

    // Getter (thread-safe, wait-free)
    T get() const
    { return data_[threadIndex()]; }

    T* getPtr()
    { return &data_[threadIndex()]; }

    // Setter (thread-safe, wait-free)
    void set( const T& value )
    { data_[threadIndex()] = value; }

  private:
    // Nb of threads that have registered
    int nb_threads_;

    // The actual data array (one element per thread from 0 to nb_threads_)
    T data_[MAX_THREADS];

    mutable QAtomicInt thread_ids_[MAX_THREADS+1];

    int threadIndex() const {

        const int thread_id = std::hash<Qt::HANDLE>()( QThread::currentThreadId() );

        int i;
        for ( i=0; thread_ids_[i]; ++i ) {
            if ( thread_ids_[i].loadAcquire() == thread_id ) { return i; }
        }

        // Current thread is missing, let's add it
        while ( i < MAX_THREADS ) {
            size_t expected = 0;
            if ( thread_ids_[i++].testAndSetOrdered( expected, thread_id ) ) {
                LOG(logDEBUG) << "Created thread for " << thread_id << " at index " << i-1;
                return i-1;
            }
        }

        assert( true );
        return 0;
    }
};

#endif
