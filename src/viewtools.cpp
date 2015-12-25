/*
 * Copyright (C) 2015 Nicolas Bonnefon and other contributors
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

#include "viewtools.h"

#include "log.h"

/* ElasticHook */

void ElasticHook::move( int value )
{
    if ( timer_id_ == 0 )
        timer_id_ = startTimer( TIMER_PERIOD_MS );

    position_ += value;
    if ( position_ <= 0 )
        position_ = 0;

    LOG( logDEBUG ) << "ElasticHook::move: new value " << position_;

    emit lengthChanged();
}

void ElasticHook::timerEvent( QTimerEvent* event )
{
    static constexpr int SQUARE_RATIO = 80;

    position_ -= DECREASE_RATE + ( ( position_/SQUARE_RATIO ) * ( position_/SQUARE_RATIO ) );

    if ( position_ <= 0 ) {
        position_ = 0;
        killTimer( timer_id_ );
        timer_id_ = 0;
    }
    LOG( logDEBUG ) << "ElasticHook::timerEvent: new value " << position_;

    emit lengthChanged();
}
