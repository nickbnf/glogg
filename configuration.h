/*
 * Copyright (C) 2009, 2010 Nicolas Bonnefon and other contributors
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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QFont>
#include <QSettings>

#include "filterset.h"

// Type of regexp to use for searches
enum SearchRegexpType {
    ExtendedRegexp,
    BasicRegexp,
    Wildcard,
    FixedString,
};

// Configuration class created as a singleton
// Accessed with Config()
class Configuration {
  public:
    // Accesses the main font used for display
    QFont mainFont() const;
    void setMainFont( QFont newFont );

    // Accesses the regexp types
    SearchRegexpType mainRegexpType() const
    { return mainRegexpType_; }
    SearchRegexpType quickfindRegexpType() const
    { return quickfindRegexpType_; }
    void setMainRegexpType( SearchRegexpType type )
    { mainRegexpType_ = type; }
    void setQuickfindRegexpType( SearchRegexpType type )
    { quickfindRegexpType_ = type; }

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
    SearchRegexpType mainRegexpType_;
    SearchRegexpType quickfindRegexpType_;
    FilterSet filterSet_;

    // allow this function to create one instance
    friend Configuration& Config();
};

Configuration& Config();


#endif
