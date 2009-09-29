/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
 *
 * This file is part of LogCrawler.
 *
 * LogCrawler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LogCrawler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LogCrawler.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QFont>
#include <QSettings>

#include "filterset.h"

// Configuration class created as a singleton
// Accessed with Config()
class Configuration {
  public:
    // Accesses the main font used for display
    QFont mainFont() const;
    void setMainFont( QFont newFont );

    // Accesses the (unique) filterSet
    FilterSet& filterSet();

    // Reads/writes the current config in the QSettings object passed
    void readFromSettings( QSettings& settings );
    void writeToSettings( QSettings& settings ) const;

  private:
    Configuration();
    Configuration( const Configuration& );

    // Configuration settings
    QFont mainFont_;
    FilterSet filterSet_;

    // allow this function to create one instance
    friend Configuration& Config();
};

Configuration& Config();

#endif
