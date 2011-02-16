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

#ifndef RECENTFILES_H
#define RECENTFILES_H

#include <QString>
#include <QStringList>

#include "persistable.h"

// Manage the list of recently opened files
class RecentFiles : public Persistable
{
  public:
    // Creates an empty set of recent files
    RecentFiles();

    // Adds the passed filename to the list of recently used searches
    void addRecent( const QString& text );

    // Returns a list of recent files (latest loaded first)
    QStringList recentFiles() const;

    // Reads/writes the current config in the QSettings object passed
    virtual void saveToStorage( QSettings& settings ) const;
    virtual void retrieveFromStorage( QSettings& settings );

  private:
    static const int RECENTFILES_VERSION;
    static const int MAX_NUMBER_OF_FILES;

    QStringList recentFiles_;
};

#endif
