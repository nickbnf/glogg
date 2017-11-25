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

#ifndef EXTERNALCOM_H
#define EXTERNALCOM_H

#include <QObject>

class CantCreateExternalErr {};

/*
 * Virtual class representing another instance of glogg.
 * Sending messages to an object of this class will forward
 * them to the instance using the underlying IPC.
 */
class ExternalInstance
{
  public:
    ExternalInstance() {}
    virtual ~ExternalInstance() {}

    virtual void loadFile( const QString& file_name ) const = 0;
    virtual uint32_t getVersion() const = 0;
};

/*
 * Class receiving messages from another instance of glogg.
 * Messages are forwarded to the application by signals.
 */
class ExternalCommunicator : public QObject
{
  Q_OBJECT

  public:
    ExternalCommunicator() : QObject() {}

    virtual ExternalInstance* otherInstance() const = 0;

    /* Instruct the communicator to start listening for
     * remote initiated operations */
    virtual void startListening() = 0;

  signals:
    void loadFile( const QString& file_name );

  public slots:
    virtual qint32 version() const = 0;
};

#endif
