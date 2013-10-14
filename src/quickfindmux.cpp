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

#include "persistentinfo.h"
#include "configuration.h"
#include "quickfindmux.h"

QuickFindMux::QuickFindMux( const QuickFindMuxSelectorInterface* selector ) :
    QObject(), pattern_()
{
    // The selector object we will use when forwarding search requests
    selector_ = selector;

    // Forward the pattern's signal to our listeners
    connect( &pattern_, SIGNAL( patternUpdated() ),
             this, SLOT( notifyPatternChanged() ) );
}

//
// Public member functions
//
void QuickFindMux::registerSearchable( QObject* searchable )
{
    // The searchable can change our qf pattern
    connect( searchable,
             SIGNAL( changeQuickFind( const QString&, QuickFindMux::QFDirection ) ),
             this, SLOT( changeQuickFind( const QString&, QuickFindMux::QFDirection ) ) );
    // Send us notifications
    connect( searchable, SIGNAL( notifyQuickFind( const QFNotification& ) ),
             this, SIGNAL( notify( const QFNotification& ) ) );
    // And clear them
    connect( searchable, SIGNAL( clearQuickFindNotification() ),
             this, SIGNAL( clearNotification() ) );
    // Search can be initiated by the view itself
    connect( searchable, SIGNAL( searchNext() ),
             this, SLOT( searchNext() ) );
    connect( searchable, SIGNAL( searchPrevious() ),
             this, SLOT( searchPrevious() ) );
}

void QuickFindMux::unregisterSearchable( QObject* searchable )
{
    disconnect( searchable );
}

void QuickFindMux::setDirection( QFDirection direction )
{
    LOG(logDEBUG) << "QuickFindMux::setDirection: new direction: " << direction;
    currentDirection_ = direction;
}

//
// Public slots
//
void QuickFindMux::searchNext()
{
    LOG(logDEBUG) << "QuickFindMux::searchNext";
    if ( currentDirection_ == Forward )
        searchForward();
    else
        searchBackward();
}

void QuickFindMux::searchPrevious()
{
    LOG(logDEBUG) << "QuickFindMux::searchPrevious";
    if ( currentDirection_ == Forward )
        searchBackward();
    else
        searchForward();
}

void QuickFindMux::searchForward()
{
    LOG(logDEBUG) << "QuickFindMux::searchForward";
    SearchableWidgetInterface* searchable = getSearchableWidget();

    searchable->searchForward();
}

void QuickFindMux::searchBackward()
{
    LOG(logDEBUG) << "QuickFindMux::searchBackward";
    SearchableWidgetInterface* searchable = getSearchableWidget();

    searchable->searchBackward();
}

void QuickFindMux::setNewPattern(
        const QString& new_pattern, bool ignore_case )
{
    static Configuration& config = Persistent<Configuration>( "settings" );

    LOG(logDEBUG) << "QuickFindMux::setNewPattern";
    pattern_.changeSearchPattern( new_pattern, ignore_case );

    // If we must do an incremental search, we do it now
    if ( config.isQuickfindIncremental() ) {
        SearchableWidgetInterface* searchable = getSearchableWidget();
        if ( currentDirection_ == Forward )
            searchable->incrementallySearchForward();
        else
            searchable->incrementallySearchBackward();
    }
}

void QuickFindMux::confirmPattern(
        const QString& new_pattern, bool ignore_case )
{
    static Configuration& config = Persistent<Configuration>( "settings" );

    pattern_.changeSearchPattern( new_pattern, ignore_case );

    // if non-incremental, we perform the search now
    if ( ! config.isQuickfindIncremental() ) {
        searchNext();
    }
    else {
        SearchableWidgetInterface* searchable = getSearchableWidget();
        searchable->incrementalSearchStop();
    }
}

void QuickFindMux::cancelSearch()
{
    static Configuration& config = Persistent<Configuration>( "settings" );

    if ( config.isQuickfindIncremental() ) {
        SearchableWidgetInterface* searchable = getSearchableWidget();
        searchable->incrementalSearchAbort();
    }
}

//
// Private slots
//
void QuickFindMux::changeQuickFind(
        const QString& new_pattern, QFDirection new_direction )
{
    pattern_.changeSearchPattern( new_pattern );
    setDirection( new_direction );
}

void QuickFindMux::notifyPatternChanged()
{
    emit patternChanged( pattern_.getPattern() );
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
