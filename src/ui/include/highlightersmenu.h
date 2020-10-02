/*
 * Copyright (C) 2020 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HIGHLIGHTERSMENU_H
#define HIGHLIGHTERSMENU_H

#include "highlighterset.h"

#include <QAction>
#include <QActionGroup>
#include <QMenu>

inline void populateHighlightersMenu( QMenu* highlightersMenu,
                                      QActionGroup* highlightersActionGroup )
{
    const auto& highlightersCollection = HighlighterSetCollection::get();
    const auto& highlighterSets = highlightersCollection.highlighterSets();
    const auto currentSetId = highlightersCollection.currentSetId();

    auto noneAction = highlightersMenu->addAction( "None" );
    noneAction->setActionGroup( highlightersActionGroup );
    noneAction->setCheckable( true );
    noneAction->setChecked( !highlightersCollection.hasSet( currentSetId ) );

    highlightersMenu->addSeparator();
    for ( const auto& highlighter : qAsConst( highlighterSets ) ) {
        auto setAction = highlightersMenu->addAction( highlighter.name() );
        setAction->setActionGroup( highlightersActionGroup );
        setAction->setCheckable( true );
        setAction->setChecked( currentSetId == highlighter.id() );
        setAction->setData( highlighter.id() );
    }
}

inline void setCurrentHighlighterAction( QActionGroup* highlightersActionGroup )
{
    const auto& highlightersCollection = HighlighterSetCollection::get();
    const auto currentSetId = highlightersCollection.currentSetId();

    const auto selectNone = !highlightersCollection.hasSet( currentSetId );

    for ( auto* action : highlightersActionGroup->actions() ) {
        const auto actionSet = action->data().toString();
        action->setChecked( currentSetId == actionSet || ( actionSet.isEmpty() && selectNone ) );
    }
}

inline void saveCurrentHighlighterFromAction( const QAction* action )
{
    auto setId = action->data().toString();
    auto& highlighterSets = HighlighterSetCollection::get();
    highlighterSets.setCurrentSet( setId );
    highlighterSets.save();
}

#endif
