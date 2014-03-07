/*
 * Copyright (C) 2014 Nicolas Bonnefon and other contributors
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

#ifndef DBUSEXTERNALCOM_H
#define DBUSEXTERNALCOM_H

#include "externalcom.h"

#include <memory>
#include <QtDBus/QtDBus>

// An implementation of ExternalInstance using D-Bus via Qt
class DBusExternalInstance : public ExternalInstance {
  public:
    DBusExternalInstance();
    ~DBusExternalInstance() {}

    virtual void loadFile( const std::string& file_name ) const;

  private:
    std::shared_ptr<QDBusInterface> dbusInterface_;
};

// An implementation of ExternalCommunicator using D-Bus via Qt
class DBusExternalCommunicator : public ExternalCommunicator
{
  public:
    // Constructor: initialise the D-Bus connection,
    // can throw if D-Bus is not available
    DBusExternalCommunicator();
    ~DBusExternalCommunicator() {}

    virtual ExternalInstance* otherInstance() const;

  signals:
    void loadFile( const std::string& file_name );

  public slots:
    QString version() const;

  private:
};

#endif
