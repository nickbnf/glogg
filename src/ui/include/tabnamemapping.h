/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov and other contributors
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
#ifndef KLOGG_TABNAMEMAPPING_H
#define KLOGG_TABNAMEMAPPING_H

#include <QString>

#include "persistable.h"


// Manage the list of recently opened files
class TabNameMapping final : public Persistable<TabNameMapping, session_settings>
{
  public:
    TabNameMapping& setTabName(const QString& path, const QString& name);
    QString tabName(const QString& path) const;

    // Reads/writes the current config in the QSettings object passed
    void saveToStorage( QSettings& settings ) const;
    void retrieveFromStorage( QSettings& settings );

  private:
    struct TabName
    {
        QString path;
        QString name;
    };

    std::vector<TabName> tabNames_;
};

#endif // KLOGG_TABNAMEMAPPING_H
