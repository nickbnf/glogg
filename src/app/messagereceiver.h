#ifndef MESSAGERECEIVER_H
#define MESSAGERECEIVER_H

#include <QtCore/QObject>
#include <QtCore/QString>

#include "log.h"

/*
 * Class receiving messages from another instance of glogg.
 * Messages are forwarded to the application by signals.
 */
class MessageReceiver : public QObject
{
  Q_OBJECT

  public:
    MessageReceiver() : QObject() {}

  signals:
    void loadFile( const QString& filename );

  public slots:
    void receiveMessage( quint32 instanceId, QByteArray message )
    {
        LOG(logINFO) << "Message from  " << instanceId;

        Q_UNUSED( instanceId );
        emit loadFile( QString::fromUtf8(message) );
    }
};

#endif // MESSAGERECEIVER_H
