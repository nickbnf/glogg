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

#include "winexternalcom.h"

#include <QString>
#include <QRegExp>

#include "log.h"

#define GLOGG_UUID "45394779-2422-49BE-AEC5-A0541FAE3127"

const QString WINDOW_TITLE = "glogg-" GLOGG_VERSION "-" GLOGG_UUID;

WinExternalCommunicator::WinExternalCommunicator()
{
    mutex_held_elsewhere_ = false;
    message_listener_     = nullptr;

    (void)::CreateMutex( NULL, TRUE, TEXT( GLOGG_UUID ) );
    switch ( ::GetLastError() ) {
        case ERROR_SUCCESS:
            LOG(logINFO) << "Mutex returned ERROR_SUCCESS";
            break;
        case ERROR_ALREADY_EXISTS:
            mutex_held_elsewhere_ = true;
            LOG(logINFO) << "Mutex returned ERROR_ALREADY_EXISTS";
            break;
        default:
            break;
    }
}

void WinExternalCommunicator::startListening()
{
    LOG(logDEBUG) << "startListening";

    message_listener_ = std::make_shared<WinMessageListener>();

    // Horrible Hack!!!
    // Why is it necessary I have no idea, but if winId is not read,
    // the message_listener_ is never called by Windows.
    // If some Windows expert could explain me....
    volatile WId id = message_listener_->winId();
    LOG(logINFO) << "Listener winID = " << id;

    connect( message_listener_.get(),
            SIGNAL( messageReceived( MessageId, const QString& ) ),
            this,
            SLOT( handleMessageReceived( MessageId, const QString& ) ) );
}

void WinExternalCommunicator::handleMessageReceived(
        MessageId message_id, const QString& data )
{
    LOG(logDEBUG) << "handleMessageReceived";

    switch ( message_id ) {
        case MessageId::LOAD_FILE:
            emit loadFile( data );
            break;
        default:
            LOG( logWARNING ) << "Unrecognised IPC message: "
                << static_cast<int>( message_id );
    }
}

ExternalInstance* WinExternalCommunicator::otherInstance() const
{
    if ( mutex_held_elsewhere_ )
        return static_cast<ExternalInstance*>( new WinExternalInstance() );
    else {
        LOG(logINFO) << "Cannot find external correspondant, we are the only glogg out there.";
        return nullptr;
    }
}

qint32 WinExternalCommunicator::version() const
{
    return 3;
}

WinExternalInstance::WinExternalInstance()
{
    LPCWSTR window_title = (LPCWSTR) WINDOW_TITLE.utf16();
    window_handle_ = FindWindow( NULL, window_title );

    LOG(logDEBUG) << "Window handle = " << window_handle_;
}

void WinExternalInstance::loadFile( const QString& file_name ) const
{
    COPYDATASTRUCT data;

    LOG(logDEBUG) << "loadFile: " << file_name.toStdString();

    data.dwData = static_cast<ULONG_PTR>( MessageId::LOAD_FILE );
    data.lpData = const_cast<char*>( file_name.toUtf8().constData() );
    data.cbData = file_name.length();

    SendMessage( window_handle_, WM_COPYDATA, 0, (LPARAM) &data );

    QString file_name2 = QString::fromLatin1(
            static_cast<const char*>( data.lpData ), data.cbData );
    LOG(logDEBUG) << "length: " << data.cbData;
    LOG(logDEBUG) << "loadFile2: " << file_name2.toStdString();

}

uint32_t WinExternalInstance::getVersion() const
{
    return 6;
}

/*
 * WinMessageListener class
 */
WinMessageListener::WinMessageListener() : QWidget()
{
    setWindowTitle( WINDOW_TITLE );
}

bool WinMessageListener::winEvent( MSG* message, long* result )
{
    if( message->message == WM_COPYDATA ) {
        // Extract the data from Windows' lParam
        COPYDATASTRUCT* data = (COPYDATASTRUCT *) message->lParam;

        LOG(logDEBUG) << "data->dwData: " << data->dwData;
        LOG(logDEBUG) << "data->cbData: " << data->cbData;
        LOG(logDEBUG) << "data->lpData: " << (const char*) data->lpData;

        QString file_name = QString::fromLatin1(
                static_cast<const char*>( data->lpData ), data->cbData );

        LOG(logINFO) << "WM_COPYDATA received, param: " << file_name.toStdString();

        emit messageReceived(
                static_cast<MessageId>( data->dwData ), file_name );

        // We process the event here
        *result = 0;
        return true;
    }
    else {
        // Give the event to qt
        return false;
    }
}
