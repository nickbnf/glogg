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

#include <chrono>

// This class is a counter that remembers the number of events occuring within one second
// it can be used for performance measurement (e.g. FPS)

class PerfCounter {
  public:
    PerfCounter() {}

    // Count a new event, returns true if it has been counted.
    // If the function returns false, it indicates the current second is elapsed
    // and the user should read and reset the counter before re-adding the event.
    bool addEvent() {
        using namespace std::chrono;
        if ( counter_ == 0 )
            first_event_date_ = steady_clock::now();

        if ( duration_cast<microseconds>(
                    steady_clock::now() - first_event_date_ ).count() < 1000000 ) {
            ++counter_;
            return true;
        }
        else {
            return false;
        }
    }

    // Read and reset the counter. Returns the number of events that occured in the
    // previous second.
    uint32_t readAndReset() {
        uint32_t value = counter_;
        counter_ = 0;
        return value;
    }

  private:
    uint32_t counter_ = 0;
    std::chrono::steady_clock::time_point first_event_date_;
};
