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

#ifndef PERSISTABLE_H
#define PERSISTABLE_H

class QSettings;

// Must be implemented by classes which could be saved to persistent
// storage by PersistentInfo.
class Persistable {
  public:
    virtual ~Persistable() {}

    // Must be implemented to save/retrieve from Qt Settings
    virtual void saveToStorage( QSettings& settings ) const = 0;
    virtual void retrieveFromStorage( QSettings& settings ) = 0;
};

#endif
