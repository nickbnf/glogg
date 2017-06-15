/*
 * Copyright (C) 2009, 2010 Nicolas Bonnefon and other contributors
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

#include "infoline.h"

#include <QPainter>

// This file implements InfoLine. It is responsible for decorating the
// widget and managing the completion gauge.

InfoLine::InfoLine() :
    QLabel(), origPalette_( palette() ),
    backgroundColor_( origPalette_.color( QPalette::Button ) ),
    darkBackgroundColor_( origPalette_.color( QPalette::Dark ) )
{
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setTextInteractionFlags(Qt::TextSelectableByMouse |
                            Qt::TextSelectableByKeyboard);
}

void InfoLine::displayGauge( int completion )
{
    int changeoverX = width() * completion / 100;

    // Create a gradient for the progress bar
    QLinearGradient linearGrad( changeoverX - 1, 0, changeoverX + 1, 0 );
    linearGrad.setColorAt( 0, darkBackgroundColor_ );
    linearGrad.setColorAt( 1, backgroundColor_ );

    // Apply the gradient to the current palette (background)
    QPalette newPalette = origPalette_;
    newPalette.setBrush( backgroundRole(), QBrush( linearGrad ) );
    setPalette( newPalette );
}

void InfoLine::hideGauge()
{
    setPalette( origPalette_ );
}

// Custom painter: draw the background then call QLabel's painter
void InfoLine::paintEvent( QPaintEvent* paintEvent )
{
    // Fill the widget background
    {
        QPainter painter( this );
        painter.fillRect( 0, 0, this->width(), this->height(),
                palette().brush( backgroundRole() ) );
    }

    // Call the parent's painter
    QLabel::paintEvent( paintEvent );
}
