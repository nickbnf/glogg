#ifndef SOCKETEXTERNALCOM_H
#define SOCKETEXTERNALCOM_H

#include "externalcom.h"

#include <QLocalServer>
#include <QSharedMemory>

class SocketExternalInstance : public ExternalInstance
{
public:
    SocketExternalInstance();

    void loadFile( const QString& file_name ) const override;
    uint32_t getVersion() const override;
private:
    QSharedMemory* memory_;
};

class SocketExternalCommunicator : public ExternalCommunicator
{
    Q_OBJECT
public:
    SocketExternalCommunicator();
    ~SocketExternalCommunicator();

    ExternalInstance* otherInstance() const override;
    void startListening() override;

public slots:
    qint32 version() const override;

private slots:
    void onConnection();
    void onReadyRead();

private:
     QSharedMemory* memory_;
     QLocalServer* server_;
};

#endif // SOCKETEXTERNALCOM_H
