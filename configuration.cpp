/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
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
}

Configuration& Config()
{
    static Configuration conf;
    return conf;
}

// Accessor functions
QFont Configuration::mainFont() const
{
    return mainFont_;
}

void Configuration::setMainFont( QFont newFont )
{
    mainFont_ = newFont;
}

FilterSet& Configuration::filterSet()
{
    return filterSet_;
}

void Configuration::readFromSettings( QSettings& settings )
{
    QString family = settings.value( "mainFont.family" ).toString();
    int size = settings.value( "mainFont.size" ).toInt();
    mainFont_ = QFont( family, size);

    filterSet_ = settings.value( "filterSet" ).value<FilterSet>();
}

void Configuration::writeToSettings( QSettings& settings ) const
{
    QFontInfo fi(mainFont_);

    settings.setValue( "mainFont.family", fi.family() );
    settings.setValue( "mainFont.size", fi.pointSize() );
    settings.setValue( "filterSet", QVariant::fromValue( filterSet_ ) );
}
