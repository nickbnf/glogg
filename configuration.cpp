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

#include "configuration.h"

Configuration::Configuration()
{
}

Configuration& Config()
{
    static Configuration conf;
    return conf;
}

// Accessor functions
QFont Configuration::mainFont() const
{
    return m_mainFont;
}

void Configuration::setMainFont(QFont newFont)
{
    m_mainFont = newFont;
}

// Read/Write functions
void Configuration::read(QSettings& settings)
{
    QString family = settings.value("mainFont.family").toString();
    int size = settings.value("mainFont.size").toInt();
    m_mainFont = QFont(family, size);
}

void Configuration::write(QSettings& settings)
{
    settings.setValue("mainFont.family", m_mainFont.family());
    settings.setValue("mainFont.size", m_mainFont.pointSize());
}
