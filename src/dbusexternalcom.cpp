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

#include "dbusexternalcom.h"

#include <QString>

#include "log.h"

static const char* DBUS_SERVICE_NAME = "org.bonnefon.glogg";

DBusExternalCommunicator::DBusExternalCommunicator()
{
    if (!QDBusConnection::sessionBus().isConnected()) {
        LOG(logERROR) << "Cannot connect to the D-Bus session bus.\n"
                    << "To start it, run:\n"
                    << "\teval `dbus-launch --auto-syntax`\n";
        throw CantCreateExternalErr();
    }

    dbus_iface_object_ = std::make_shared<DBusInterfaceExternalCommunicator>();
}

void DBusExternalCommunicator::startListening()
{
    if (!QDBusConnection::sessionBus().registerService( DBUS_SERVICE_NAME )) {
        LOG(logERROR) << qPrintable(QDBusConnection::sessionBus().lastError().message());
        throw CantCreateExternalErr();
    }

    if ( !QDBusConnection::sessionBus().registerObject( "/",
            dbus_iface_object_.get(), QDBusConnection::ExportAllContents ) ) {
        LOG(logERROR) << qPrintable(QDBusConnection::sessionBus().lastError().message());
        throw CantCreateExternalErr();
    }
}

ExternalInstance* DBusExternalCommunicator::otherInstance() const
{
    try {
        return static_cast<ExternalInstance*>( new DBusExternalInstance() );
    }
    catch ( CantCreateExternalErr ) {
        LOG(logINFO) << "Cannot find external D-Bus correspondant, we are the only glogg out there.";
        return nullptr;
    }
}

qint32 DBusExternalCommunicator::version() const
{
    return 3;
}

qint32 DBusInterfaceExternalCommunicator::version() const
{
    return 0x010000;
}

void DBusInterfaceExternalCommunicator::loadFile( const QString& file_name )
{
    LOG(logDEBUG) << "DBusInterfaceExternalCommunicator::loadFile()";

    emit signalLoadFile( file_name );
}

DBusExternalInstance::DBusExternalInstance()
{
     dbusInterface_ = std::make_shared<QDBusInterface>(
             DBUS_SERVICE_NAME, "/", "", QDBusConnection::sessionBus() );

     if ( ! dbusInterface_->isValid() ) {
        throw CantCreateExternalErr();
     }
}

void DBusExternalInstance::loadFile( const std::string& file_name ) const
{
    QDBusReply<QString> reply =
        dbusInterface_->call( "loadFile", QString( file_name.c_str() ) );

    if ( ! reply.isValid() ) {
        LOG( logWARNING ) << "Invalid reply from D-Bus call: "
            << qPrintable( reply.error().message() );
    }
}

uint32_t DBusExternalInstance::getVersion() const
{
    QDBusReply<qint32> reply = dbusInterface_->call( "version" );

    if ( ! reply.isValid() ) {
        LOG( logWARNING ) << "Invalid reply from D-Bus call: "
            << qPrintable( reply.error().message() );
        return 0;
    }

    return (uint32_t) reply.value();
}
