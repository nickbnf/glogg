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

#ifndef RECENTENCODINGS_H
#define RECENTENCODINGS_H

#include <QString>
#include <QStringList>

#include "persistable.h"

// Manage the list of recently used encodings
class RecentEncodings : public Persistable
{
  public:
    // Creates an empty set of recent encodings
    RecentEncodings();

    // Maximum encodings that are saved before rolling over
    int max();

    // Adds the passed encoding to the list of recently used encodings
    void addRecent( const QString& encoding );

    // Returns a list of recent encodings (most recent first)
    QStringList recentEncodings() const;

    // Reads/writes the current encoding list into the passed QSettings object
    virtual void saveToStorage( QSettings& settings ) const;
    virtual void retrieveFromStorage( QSettings& settings );

  private:
    static const int RECENTENCODINGS_VERSION;
    static const int MAX_NUMBER_OF_ENCODINGS;

    QStringList recentEncodings_;
};

#endif
