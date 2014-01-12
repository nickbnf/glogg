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

#include "signalmux.h"

#include <algorithm>
#include <QObject>

#include "log.h"

SignalMux::SignalMux() :
    connectionList_()
{
    currentDocument_ = nullptr;
}

void SignalMux::connect( QObject *sender,
        const char *signal, const char *slot )
{
    Connection new_connection = {
            sender,
            nullptr,
            signal,
            slot };

    connectionList_.push_back( new_connection );

    connect( new_connection );
}

void SignalMux::disconnect( QObject *sender,
        const char *signal, const char *slot )
{
    // Find any signal that match our description
    auto connection = std::find_if(
            connectionList_.begin(), connectionList_.end(),
            [sender, signal, slot]( const Connection& c ) -> bool
            { return ((QObject*)c.source == sender) && (c.sink == nullptr) &&
                (qstrcmp( c.signal, signal) == 0) && (qstrcmp( c.slot, slot) == 0); } );

    if ( connection != connectionList_.end() )
    {
        disconnect( *connection );

        connectionList_.erase( connection );
    }
    else
    {
        LOG( logERROR ) << "SignalMux disconnecting a non-existing signal";
    }
}

// Upstream signals
void SignalMux::connect( const char* signal,
        QObject* receiver, const char* slot )
{
    Connection new_connection = {
            nullptr,
            receiver,
            signal,
            slot };

    connectionList_.push_back( new_connection );

    connect( new_connection );
}

void SignalMux::disconnect( const char *signal,
        QObject* receiver, const char *slot )
{
    // Find any signal that match our description
    auto connection = std::find_if(
            connectionList_.begin(), connectionList_.end(),
            [receiver, signal, slot]( const Connection& c ) -> bool
            { return ((QObject*)c.sink == receiver) && (c.source == nullptr) &&
                (qstrcmp( c.signal, signal) == 0) && (qstrcmp( c.slot, slot) == 0); } );

    if ( connection != connectionList_.end() )
    {
        disconnect( *connection );

        connectionList_.erase( connection );
    }
    else
    {
        LOG( logERROR ) << "SignalMux disconnecting a non-existing signal";
    }
}

void SignalMux::setCurrentDocument( QObject* current_document )
{
    // First disconnect everything to/from the old document
    for ( auto c: connectionList_ )
        disconnect( c );

    currentDocument_ = current_document;

    // And now connect to/from the new document
    for ( auto c: connectionList_ )
        connect( c );

    // And ask the doc to emit all state signals
    MuxableDocumentInterface* doc =
        dynamic_cast<MuxableDocumentInterface*>( current_document );
    if ( doc )
        doc->sendAllStateSignals();
}

/*
 * Private functions
 */
void SignalMux::connect( const Connection& connection )
{
    if ( currentDocument_ )
    {
        LOG( logDEBUG ) << "SignalMux::connect";
        if ( connection.source && ( ! connection.sink ) )
        {
            // Downstream signal
            QObject::connect( connection.source, connection.signal,
                    currentDocument_, connection.slot );
        }
        else if ( ( ! connection.source ) && connection.sink )
        {
            // Upstream signal
            QObject::connect( currentDocument_, connection.signal,
                    connection.sink, connection.slot );
        }
        else
        {
            LOG( logERROR ) << "SignalMux has an unexpected signal";
        }
    }
}

void SignalMux::disconnect( const Connection& connection )
{
    if ( currentDocument_ )
    {
        LOG( logDEBUG ) << "SignalMux::disconnect";
        if ( connection.source && ( ! connection.sink ) )
        {
            // Downstream signal
            QObject::disconnect( connection.source, connection.signal,
                    currentDocument_, connection.slot );
        }
        else if ( ( ! connection.source ) && connection.sink )
        {
            // Upstream signal
            QObject::disconnect( currentDocument_, connection.signal,
                    connection.sink, connection.slot );
        }
        else
        {
            LOG( logERROR ) << "SignalMux has an unexpected signal";
        }
    }
}
