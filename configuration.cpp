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

#include <QFontInfo>

#include "log.h"

#include "configuration.h"

Configuration::Configuration()
{
    // Should have some sensible default values.
    mainFont_ = QFont("mono", 10);
    mainFont_.setStyleHint( QFont::Courier, QFont::PreferOutline );

    mainRegexpType_      = ExtendedRegexp;
    quickfindRegexpType_ = ExtendedRegexp;

    overviewVisible_     = true;
    lineNumbersVisible_  = false;

    QFontInfo fi(mainFont_);
    LOG(logDEBUG) << "Default font is " << fi.family().toStdString();
}

// Accessor functions
QFont Configuration::mainFont() const
{
    return mainFont_;
}

void Configuration::setMainFont( QFont newFont )
{
    LOG(logDEBUG) << "Configuration::setMainFont";

    mainFont_ = newFont;
}

void Configuration::retrieveFromStorage( QSettings& settings )
{
    LOG(logDEBUG) << "Configuration::retrieveFromStorage";

    // Fonts
    QString family = settings.value( "mainFont.family" ).toString();
    int size = settings.value( "mainFont.size" ).toInt();

    // If no config read, keep the default
    if ( !family.isNull() )
        mainFont_ = QFont( family, size );

    // Regexp types
    mainRegexpType_ = static_cast<SearchRegexpType>(
            settings.value( "regexpType.main", mainRegexpType_ ).toInt() );
    quickfindRegexpType_ = static_cast<SearchRegexpType>(
            settings.value( "regexpType.quickfind", quickfindRegexpType_ ).toInt() );

    // View settings
    if ( settings.contains( "view.overviewVisible" ) )
        overviewVisible_ = settings.value( "view.overviewVisible" ).toBool();
    if ( settings.contains( "view.lineNumbersVisible" ) )
        lineNumbersVisible_ = settings.value( "view.lineNumbersVisible" ).toBool();
}

void Configuration::saveToStorage( QSettings& settings ) const
{
    LOG(logDEBUG) << "Configuration::saveToStorage";

    QFontInfo fi(mainFont_);

    settings.setValue( "mainFont.family", fi.family() );
    settings.setValue( "mainFont.size", fi.pointSize() );
    settings.setValue( "regexpType.main", static_cast<int>( mainRegexpType_ ) );
    settings.setValue( "regexpType.quickfind", static_cast<int>( quickfindRegexpType_ ) );
    settings.setValue( "view.overviewVisible", overviewVisible_ );
    settings.setValue( "view.lineNumbersVisible", lineNumbersVisible_ );
}
