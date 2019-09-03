/*
 * Copyright (C) 2011, 2014 Nicolas Bonnefon and other contributors
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

#ifndef SESSIONINFO_H
#define SESSIONINFO_H

#include <vector>
#include <string>

#include <QByteArray>
#include <QString>

#include "persistable.h"

// Simple component class containing information related to the session
// to be persisted and reloaded upon start
class SessionInfo : public Persistable<SessionInfo, session_settings> {
  public:
    SessionInfo() : openFiles_() { }

    // Geometry of the main window
    // (this is an opaque string which is interpreted by the
    // MainWindow implementation)
    QByteArray geometry() const
    { return geometry_; }
    void setGeometry( const QByteArray& geometry )
    { geometry_ = geometry; }

    struct OpenFile
    {
        OpenFile( const QString& file, uint64_t top, const QString& context )
            : fileName{ file }
            , topLine{ top }
            , viewContext{ context }
        {}

        QString fileName;
        uint64_t    topLine;
        // The view context contains parameter specific to the view's
        // implementation (such as geometry...)
        QString viewContext;
    };

    // List of the loaded files
    std::vector<OpenFile> openFiles() const
    { return openFiles_; }
    void setOpenFiles( const std::vector<OpenFile>& loaded_files )
    { openFiles_ = loaded_files; }

    // Reads/writes the current config in the QSettings object passed
    void saveToStorage( QSettings& settings ) const;
    void retrieveFromStorage( QSettings& settings );

  private:
    static const int OPENFILES_VERSION;

    QByteArray geometry_;
    QByteArray crawlerState_;
    std::vector<OpenFile> openFiles_;
};

#endif
