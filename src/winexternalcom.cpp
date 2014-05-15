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
            LOG(logERROR) << "ERROR_SUCCESS";
            break;
        case ERROR_ALREADY_EXISTS:
            mutex_held_elsewhere_ = true;
            LOG(logERROR) << "ERROR_ALREADY_EXISTS";
            break;
        default:
            break;
    }
}

void WinExternalCommunicator::startListening()
{
    LOG(logDEBUG) << "startListening";

    message_listener_ = std::make_shared<WinMessageListener>();

    LOG(logDEBUG) << "Listener winID = " << message_listener_->winId();

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

WINBOOL WinExternalInstance::enumWindowsCallback( HWND hwnd, LPARAM lParam )
{
    static const QRegExp window_re("blah");
    static const QRegExp module_re("glogg.exe$");
    wchar_t window_name[200] = { 0 };
    wchar_t module_name[200] = { 0 };
    auto window_handles = reinterpret_cast<std::vector<HWND>*>( lParam );

    int window_len = GetWindowText( hwnd, window_name,
            sizeof(window_name) / sizeof(window_name[0]) - 1 );
    int module_len = GetWindowModuleFileName( hwnd, module_name,
            sizeof(module_name) / sizeof(module_name[0]) - 1 );

    QString window = QString::fromWCharArray( window_name, window_len );
    QString module = QString::fromWCharArray( module_name, module_len );

    // if ( window.contains( window_re ) ) // && module.contains( module_re ) )
    {
        std::string stdUtf8 = window.toUtf8().constData();
        std::string module_u8 = module.toUtf8().constData();
        LOG(logERROR) << "enum (" << hwnd << ") window:" << stdUtf8;
        LOG(logERROR) << "enum (" << hwnd << ") module:" << module_u8;

        window_handles->push_back( hwnd );
    }

    return TRUE;
}

WinExternalInstance::WinExternalInstance()
{
    LPCWSTR window_title = (LPCWSTR) WINDOW_TITLE.utf16();
    HWND window_handle_ = FindWindow( NULL, window_title );

    std::vector<HWND> window_handles;

    // EnumWindows cannot tell us for sure which window
    // is the genuine glogg, so we store all of them.
    EnumWindows( (WNDENUMPROC) enumWindowsCallback,
            reinterpret_cast<LPARAM>( &window_handles ) );

    // LOG(logERROR) << "Window handle = " << window_handles.front();

    LOG(logERROR) << "Window handle = " << window_handle_;
}

void WinExternalInstance::loadFile( const QString& file_name ) const
{
}

uint32_t WinExternalInstance::getVersion() const
{
}

/*
 * WinMessageListener class
 */
WinMessageListener::WinMessageListener()
{
    setWindowTitle( WINDOW_TITLE );
    //setWindowTitle( "blah" );
}

bool WinMessageListener::winEvent( MSG* message, long* result )
{
    LOG(logERROR) << "winEvent";

    if( message->message == WM_COPYDATA ) {
        // Extract the data from Windows' lParam
        COPYDATASTRUCT* data = (COPYDATASTRUCT *) message->lParam;

        emit messageReceived( static_cast<MessageId>( data->dwData ),
                QString::fromAscii(
                    static_cast<const char *>( data->lpData ), data->cbData ) );

        // We process the event here
        *result = 0;
        return true;
    }
    else {
        // Give the event to qt
        return false;
    }
}
