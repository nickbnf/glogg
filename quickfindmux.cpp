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

#include "log.h"

#include "quickfindmux.h"

QuickFindMux::QuickFindMux( const QuickFindMuxSelectorInterface* selector ) :
    QObject(), pattern_()
{
    // The selector object we will use when forwarding search requests
    selector_ = selector;
}

//
// Public member functions
//
void QuickFindMux::registerSearchable( QObject* searchable )
{
    // The searchable can change our qf pattern
    connect( searchable, SIGNAL( changeQuickFind( const QString& ) ),
             this, SLOT( setNewPattern( const QString& ) ) );
    // Send us notifications
    connect( searchable, SIGNAL( notifyQuickFind( const QFNotification& ) ),
             this, SIGNAL( notify( const QFNotification& ) ) );
    // And clear them
    connect( searchable, SIGNAL( clearQuickFindNotification() ),
             this, SIGNAL( clearNotification() ) );
}

void QuickFindMux::unregisterSearchable( QObject* searchable )
{
    disconnect( searchable );
}

void QuickFindMux::setDirection( QFDirection direction )
{
    currentDirection_ = direction;
}

void QuickFindMux::setNewPattern( const QString& new_pattern )
{
    pattern_.changeSearchPattern( new_pattern );
    emit patternChanged( new_pattern );
}

void QuickFindMux::searchNext()
{
    if ( currentDirection_ == Forward )
        searchForward();
    else
        searchBackward();
}

void QuickFindMux::searchPrevious()
{
    if ( currentDirection_ == Forward )
        searchBackward();
    else
        searchForward();
}

void QuickFindMux::searchForward()
{
    SearchableWidgetInterface* searchable = getSearchableWidget();

    searchable->searchForward();
}

void QuickFindMux::searchBackward()
{
    SearchableWidgetInterface* searchable = getSearchableWidget();

    searchable->searchForward();
}

//
// Private member functions
//

// Use the registered 'selector' to determine where to send the search requests.
SearchableWidgetInterface* QuickFindMux::getSearchableWidget() const
{
    SearchableWidgetInterface* searchable = NULL;

    if ( selector_ )
        searchable = selector_->getActiveSearchable();
    else
        LOG(logERROR) << "QuickFindMux::getActiveSearchable() no registered selector";

    return searchable;
}
