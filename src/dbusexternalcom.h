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
#include <QObject>
#include <QtDBus/QtDBus>

// An implementation of ExternalInstance using D-Bus via Qt
class DBusExternalInstance : public ExternalInstance {
  public:
    DBusExternalInstance();
    ~DBusExternalInstance() {}

    virtual void loadFile( const QString& file_name ) const;
    virtual uint32_t getVersion() const;

  private:
    std::shared_ptr<QDBusInterface> dbusInterface_;
};

class DBusInterfaceExternalCommunicator : public QObject
{
  Q_OBJECT

  public:
    DBusInterfaceExternalCommunicator() : QObject() {}
    ~DBusInterfaceExternalCommunicator() {}

  public slots:
    void loadFile( const QString& file_name );
    qint32 version() const;

  signals:
    void signalLoadFile( const QString& file_name );
};

// An implementation of ExternalCommunicator using D-Bus via Qt
class DBusExternalCommunicator : public ExternalCommunicator
{
  Q_OBJECT

  public:
    // Constructor: initialise the D-Bus connection,
    // can throw if D-Bus is not available
    DBusExternalCommunicator();
    ~DBusExternalCommunicator() {}

    virtual void startListening();

    virtual ExternalInstance* otherInstance() const;

  public slots:
    qint32 version() const;

  private:
    std::shared_ptr<DBusInterfaceExternalCommunicator> dbus_iface_object_;
};

#endif
