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
    static constexpr int MAX_POSITION = 2000;

    if ( timer_id_ == 0 )
        timer_id_ = startTimer( TIMER_PERIOD_MS );

    int resistance = 0;
    if ( !held_ && ( position_ * value > 0 ) ) // value and resistance have the same sign
        resistance = position_ / 8;

    position_ = std::min( position_ + ( value - resistance ), MAX_POSITION );

    if ( !held_ && ( std::chrono::duration_cast<std::chrono::milliseconds>
            ( std::chrono::steady_clock::now() - last_update_ ).count() > TIMER_PERIOD_MS ) )
        decreasePosition();

    if ( ( ! hooked_ ) && position_ >= hook_threshold_ ) {
        position_ -= hook_threshold_;
        hooked_ = true;
        emit hooked( true );
    }
    else if ( hooked_ && position_ <= - hook_threshold_ ) {
        position_ += hook_threshold_;
        hooked_ = false;
        emit hooked( false );
    }

    if ( position_ < 0 && !isHooked() )
        position_ = 0;

    last_update_ = std::chrono::steady_clock::now();

    LOG( logDEBUG ) << "ElasticHook::move: new value " << position_;

    emit lengthChanged();
}

void ElasticHook::timerEvent( QTimerEvent* )
{
    if ( !held_ && ( std::chrono::duration_cast<std::chrono::milliseconds>
            ( std::chrono::steady_clock::now() - last_update_ ).count() > TIMER_PERIOD_MS ) ) {
        decreasePosition();
        last_update_ = std::chrono::steady_clock::now();
    }
}

void ElasticHook::decreasePosition()
{
    static constexpr int PROP_RATIO = 10;

    // position_ -= DECREASE_RATE + ( ( position_/SQUARE_RATIO ) * ( position_/SQUARE_RATIO ) );
    if ( std::abs( position_ ) < DECREASE_RATE ) {
        position_ = 0;
        killTimer( timer_id_ );
        timer_id_ = 0;
    }
    else if ( position_ > 0 )
        position_ -= DECREASE_RATE + ( position_/PROP_RATIO );
    else if ( position_ < 0 )
        position_ += DECREASE_RATE - ( position_/PROP_RATIO );

    LOG( logDEBUG ) << "ElasticHook::timerEvent: new value " << position_;

    emit lengthChanged();
}
