/*
 * Copyright (C) 2011 Nicolas Bonnefon and other contributors
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

// Use a bisection method to find the given line number
// in a sorted list.
// The T type must be a container containing elements that
// implement the lineNumber() member.
// Returns true if the lineNumber is found, false if not
// foundIndex is the index of the found number or the index
// of the closest greater element.
template <typename T> bool lookupLineNumber(
        const T& list, qint64 lineNumber, int* foundIndex )
{
    int minIndex = 0;
    int maxIndex = list.size() - 1;
    // If the list is not empty
    if ( maxIndex - minIndex >= 0 ) {
        // First we test the ends
        if ( list[minIndex].lineNumber() == lineNumber ) {
            *foundIndex = minIndex;
            return true;
        }
        else if ( list[maxIndex].lineNumber() == lineNumber ) {
            *foundIndex = maxIndex;
            return true;
        }

        // Then we test the rest
        while ( (maxIndex - minIndex) > 1 ) {
            const int tryIndex = (minIndex + maxIndex) / 2;
            const qint64 currentMatchingNumber =
                list[tryIndex].lineNumber();
            if ( currentMatchingNumber > lineNumber )
                maxIndex = tryIndex;
            else if ( currentMatchingNumber < lineNumber )
                minIndex = tryIndex;
            else if ( currentMatchingNumber == lineNumber ) {
                *foundIndex = tryIndex;
                return true;
            }
        }
    }

    *foundIndex = minIndex;
    return false;
}
