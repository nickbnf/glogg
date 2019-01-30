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
