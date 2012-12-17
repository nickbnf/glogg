/*
 * Copyright (C) 2009, 2010, 2011 Nicolas Bonnefon and other contributors
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

#include "persistable.h"

// Type of regexp to use for searches
enum SearchRegexpType {
    ExtendedRegexp,
    Wildcard,
    FixedString,
};

// Configuration class containing everything in the "Settings" dialog
class Configuration : public Persistable {
  public:
    Configuration();

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

    // View settings
    bool isOverviewVisible() const
    { return overviewVisible_; }
    void setOverviewVisible( bool isVisible )
    { overviewVisible_ = isVisible; }
    bool lineNumbersVisible() const
    { return lineNumbersVisible_; }
    void setLineNumbersVisible( bool lineNumbersVisible )
    { lineNumbersVisible_ = lineNumbersVisible; }

    // Reads/writes the current config in the QSettings object passed
    virtual void saveToStorage( QSettings& settings ) const;
    virtual void retrieveFromStorage( QSettings& settings );

  private:
    // Configuration settings
    QFont mainFont_;
    SearchRegexpType mainRegexpType_;
    SearchRegexpType quickfindRegexpType_;

    // View settings
    bool overviewVisible_;
    bool lineNumbersVisible_;
};

#endif
