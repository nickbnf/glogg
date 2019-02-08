/*
 * Copyright (C) 2019 Anton Filimonov and other contributors
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

#ifndef MESSAGERECEIVER_H
#define MESSAGERECEIVER_H

#include <QtCore/QJsonDocument>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "log.h"
#include "version.h"

/*
 * Class receiving messages from another instance of klogg.
 * Messages are forwarded to the application by signals.
 */
class MessageReceiver final : public QObject {
    Q_OBJECT

  public:
    MessageReceiver()
        : QObject()
    {
    }

  signals:
    void loadFile( const QString& filename );

  public slots:
    void receiveMessage( quint32 instanceId, QByteArray message )
    {
        auto json = QJsonDocument::fromBinaryData( message );

        LOG( logINFO ) << "Message from  " << instanceId << json.toJson().toStdString();

        Q_UNUSED( instanceId );

        QVariantMap data = json.toVariant().toMap();
        if ( data[ "version" ].toString() != GLOGG_VERSION ) {
            return;
        }

        QStringList filenames = data[ "files" ].toStringList();

        for ( const auto& f : filenames ) {
            emit loadFile( f );
        }
    }
};

#endif // MESSAGERECEIVER_H
