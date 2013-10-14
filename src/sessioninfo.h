/*
 * Copyright (C) 2011 Nicolas Bonnefon and other contributors
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

#include <QByteArray>
#include <QString>

#include "persistable.h"

// Simple component class containing information related to the session
// to be persisted and reloaded upon start
class SessionInfo : public Persistable {
  public:
    SessionInfo() { }

    // Geometry of the main window
    QByteArray geometry() const
    { return geometry_; }
    void setGeometry( const QByteArray& geometry )
    { geometry_ = geometry; }
    // Geometry of the CrawlerWidget
    QByteArray crawlerState() const
    { return crawlerState_; }
    void setCrawlerState( const QByteArray& geometry )
    { crawlerState_ = geometry; }
    // Name of the current (last loaded) file
    QString currentFile() const
    { return currentFile_; }
    void setCurrentFile( const QString& filename )
    { currentFile_ = filename; }

    // Reads/writes the current config in the QSettings object passed
    virtual void saveToStorage( QSettings& settings ) const;
    virtual void retrieveFromStorage( QSettings& settings );

  private:
    QByteArray geometry_;
    QByteArray crawlerState_;
    QString    currentFile_;
};

#endif
