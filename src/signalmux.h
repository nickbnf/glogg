/*
 * Copyright (C) 2013 Nicolas Bonnefon and other contributors
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

#ifndef SIGNALMUX_H
#define SIGNALMUX_H

// This class multiplexes Qt signals exchanged between one main
// window and several 'document' widgets.
// The main window register signals with the mux (instead of directly
// with the document) and then just notify the mux when the 'current
// document' changes
/*
   +---------------------------------------+
   |                                       |
   |              Main Window              |--+
   |                                       |  |
   +---------------------------------------+  |
                     +  ^                     |
                     v  +                     |
   +---------------------------------------+  |
   |                  MUX                  |<-+
   +---------------------------------------+
      +----------^ +
      | +----------+
      | v
   +--+-----+ +--------+ +-------+ +-------+
   |        | |        | |       | |       |
   | Doc 1  | | Doc 2  | | Doc 3 | | Doc 4 |
   |        | |        | |       | |       |
   +--------+ +--------+ +-------+ +-------+
*/
// Strongly inspired by http://doc.qt.digia.com/qq/qq08-action-multiplexer.html

#include <list>
#include <QPointer>

class QObject;

class SignalMux {
  public:
    SignalMux();

    // Connect an 'downstream' signal
    void connect(QObject *sender, const char *signal, const char *slot);
    void disconnect(QObject *sender, const char *signal, const char *slot);
    // Connect an 'upstream' signal
    void connect(const char *signal, QObject *receiver, const char *slot);
    void disconnect(const char *signal, QObject *receiver, const char *slot);

    // Change the current document
    void setCurrentDocument( QObject* current_document );

  private:
    struct Connection {
        QPointer<QObject>   source;
        QPointer<QObject>   sink;
        const char*         signal;
        const char*         slot;
    };

    void connect( const Connection& connection);
    void disconnect( const Connection& connection);

    std::list<Connection> connectionList_;

    QPointer<QObject> currentDocument_;
};

#endif
