/*
 * Copyright (C) 2020 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KLOGG_STYLES
#define KLOGG_STYLES

#include <QStyleFactory>

constexpr QLatin1String DarkStyleKey = QLatin1String( "Dark", 4 );
constexpr QLatin1String VistaKey = QLatin1String( "WindowsVista", 12 );
constexpr QLatin1String FusionKey = QLatin1String( "Fusion", 6 );
constexpr QLatin1String WindowsKey = QLatin1String( "Windows", 7 );

inline QStringList availableStyles()
{
    QStringList styles;
#ifdef Q_OS_WIN
    styles << VistaKey;
    styles << WindowsKey;
    styles << FusionKey;
#else
    styles << QStyleFactory::keys();
#endif

    styles << DarkStyleKey;

    return styles;
}

#endif
