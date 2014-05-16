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

#ifndef WINEXTERNALCOM_H
#define WINEXTERNALCOM_H

#include "externalcom.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winuser.h>
#include <windef.h>

#include <memory>
#include <QObject>
#include <QWidget>

// An implementation of ExternalInstance using Windows IPC
class WinExternalInstance : public ExternalInstance {
  public:
    WinExternalInstance();
    ~WinExternalInstance() {}

    virtual void loadFile( const QString& file_name ) const;
    virtual uint32_t getVersion() const;

  private:
    static WINBOOL enumWindowsCallback( HWND hwnd, LPARAM lParam );
    HWND window_handle_;
};

enum class MessageId {
    LOAD_FILE
};

// A hidden widget to listen for message via Windows'
// WM_COPYDATA mechanism.
// (this should be nested into WinExternalCommunicator really but Qt MOC
// doesn't support nested classes)
class WinMessageListener : public QWidget
{
  Q_OBJECT

  public:
    WinMessageListener();

  signals:
    // A message has been received from Windows
    void messageReceived( MessageId message_id, const QString& data );

  private:
    // Override the default event message
    bool winEvent( MSG* message, long* result );
};

// An implementation of ExternalCommunicator using Windows IPC
class WinExternalCommunicator : public ExternalCommunicator
{
  Q_OBJECT

  public:
    // Constructor: initialise the D-Bus connection,
    // can throw if D-Bus is not available
    WinExternalCommunicator();
    ~WinExternalCommunicator() {}

    virtual void startListening();

    virtual ExternalInstance* otherInstance() const;

  public slots:
    qint32 version() const;

  private slots:
      void handleMessageReceived(
              MessageId message_id, const QString& data );

  private:
    static const QString WINDOW_TITLE;

    bool mutex_held_elsewhere_;
    std::shared_ptr<WinMessageListener> message_listener_;
};

#endif
