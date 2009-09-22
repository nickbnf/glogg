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

/// Configuration class created as a singleton
class Configuration {
    public:
        QFont mainFont() const;
        void setMainFont(QFont newFont);

        void read(QSettings& settings);
        void write(QSettings& settings);

    private:
        Configuration();
        Configuration(const Configuration&);

        // Configuration settings
        QFont m_mainFont;

        // allow this function to create one instance
        friend Configuration& Config();
};

Configuration& Config();

#endif
