#include "socketexternalcom.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QSharedMemory>

#ifdef Q_OS_UNIX
    #include <signal.h>
    #include <unistd.h>
#endif

#include "log.h"

static const char* GLOG_SERVICE_NAME = "org.bonnefon.glogg";

#ifdef Q_OS_UNIX
QSharedMemory* g_staticSharedMemory = nullptr;

void terminate(int signum)
{
    if (g_staticSharedMemory) {
        delete g_staticSharedMemory;
    }
    ::exit(128 + signum);
}

void setCrashHandler(QSharedMemory* memory)
{
    g_staticSharedMemory = memory;

    // Handle any further termination signals to ensure the
    // QSharedMemory block is deleted even if the process crashes
    signal(SIGSEGV, terminate);
    signal(SIGABRT, terminate);
    signal(SIGFPE,  terminate);
    signal(SIGILL,  terminate);
    signal(SIGINT,  terminate);
    signal(SIGTERM, terminate);
}

#endif

SocketExternalInstance::SocketExternalInstance()
    : ExternalInstance(), memory_(new QSharedMemory(GLOG_SERVICE_NAME) )
{
    if ( !memory_->attach( QSharedMemory::ReadOnly ) ) {
        throw CantCreateExternalErr();
    }

#ifdef Q_OS_UNIX
        // Handle any further termination signals to ensure the
        // QSharedMemory block is deleted even if the process crashes
        setCrashHandler(memory_);
#endif
}

void SocketExternalInstance::loadFile(const QString &file_name) const
{
    QLocalSocket socket;
    socket.connectToServer(GLOG_SERVICE_NAME);
    if (!socket.waitForConnected(1000)) {
        LOG( logERROR ) << "Failed to connect to socket";
        return;
    }

    socket.write(file_name.toUtf8());
    if (!socket.waitForBytesWritten(1000)) {
        LOG( logERROR ) << "Failed to send filename";
    }

    socket.close();
}

uint32_t SocketExternalInstance::getVersion() const
{
    return *reinterpret_cast<uint32_t*>(memory_->data());
}

SocketExternalCommunicator::SocketExternalCommunicator()
    : ExternalCommunicator()
    , memory_(new QSharedMemory(GLOG_SERVICE_NAME))
    , server_(new QLocalServer)
{}

SocketExternalCommunicator::~SocketExternalCommunicator()
{
    delete memory_;
    server_->close();
    delete server_;
}

void SocketExternalCommunicator::startListening()
{
    if ( memory_->create(sizeof(qint32))) {
#ifdef Q_OS_UNIX
        // Handle any further termination signals to ensure the
        // QSharedMemory block is deleted even if the process crashes
        setCrashHandler(memory_);
#endif

        *reinterpret_cast<qint32*>(memory_->data()) = version();
        QLocalServer::removeServer(GLOG_SERVICE_NAME);

        connect(server_, SIGNAL(newConnection()), SLOT(onConnection()));
        server_->listen(GLOG_SERVICE_NAME);
    }
}

qint32 SocketExternalCommunicator::version() const
{
    return 3;
}

ExternalInstance* SocketExternalCommunicator::otherInstance() const
{
    try {
        return static_cast<ExternalInstance*>( new SocketExternalInstance() );
    }
    catch ( CantCreateExternalErr ) {
        LOG(logINFO) << "Cannot find external correspondant, we are the only glogg out there.";
        return nullptr;
    }
}

void SocketExternalCommunicator::onConnection()
{
     QLocalSocket *socket = server_->nextPendingConnection();
     connect(socket, SIGNAL(readyRead()), SLOT(onReadyRead()));
}

void SocketExternalCommunicator::onReadyRead()
{
    QLocalSocket* socket = qobject_cast<QLocalSocket>(sender());
    QByteArray data;
    data.append(socket->readAll());
    socket->close();

    emit loadFile(QString::fromUtf8(data));
}



