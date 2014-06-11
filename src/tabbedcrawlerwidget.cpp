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

#include "tabbedcrawlerwidget.h"

#include <QKeyEvent>

#include "log.h"

TabbedCrawlerWidget::TabbedCrawlerWidget() : QTabWidget(), myTabBar_()
{
#if WIN32
    myTabBar_.setStyleSheet( "QTabBar::tab {\
            height: 20px; "
            "}" );
#else
    // On GTK style, it looks better with a smaller font
    myTabBar_.setStyleSheet(
            "QTabBar::tab {"
            " height: 20px; "
            " font-size: 9pt; "
            "} "
            "QTabBar::close-button {\
            height: 6px; width: 6px; }" );
#endif
    setTabBar( &myTabBar_ );
    myTabBar_.hide();
}

// I know hiding non-virtual functions from the base class is bad form
// and I do it here out of pure laziness: I don't want to encapsulate
// QTabBar with all signals and all just to implement this very simple logic.
// Maybe one day that should be done better...

int TabbedCrawlerWidget::addTab( QWidget* page, const QString& label )
{
    int index = QTabWidget::addTab( page, label );

    LOG(logDEBUG) << "addTab, count = " << count();

    if ( count() > 1 )
        myTabBar_.show();

    return index;
}

void TabbedCrawlerWidget::removeTab( int index )
{
    QTabWidget::removeTab( index );

    if ( count() <= 1 )
        myTabBar_.hide();
}

void TabbedCrawlerWidget::keyPressEvent( QKeyEvent* event )
{
    LOG(logDEBUG) << "TabbedCrawlerWidget::keyPressEvent";

    char c = ( event->text() )[0].toLatin1();
    Qt::KeyboardModifiers mod = event->modifiers();

    // Ctrl + tab
    if ( mod == Qt::ControlModifier && c == '\t' ) {
        setCurrentIndex( ( currentIndex() + 1 ) % count() );
    }
    // Ctrl + shift + tab
    else if ( mod == ( Qt::ControlModifier | Qt::ShiftModifier ) && c == '\t' ) {
        setCurrentIndex( ( currentIndex() + 1 ) % count() );
    }
    // Ctrl + numbers
    else if ( mod == Qt::ControlModifier && ( c >= '1' && c <= '8' ) ) {
        LOG(logDEBUG) << "number " << c;
        int new_index = c - '0';
        if ( new_index <= count() )
            setCurrentIndex( new_index - 1 );
    }
    // Ctrl + 9
    else if ( mod == Qt::ControlModifier && c == '9' ) {
        LOG(logDEBUG) << "last";
        setCurrentIndex( count() - 1 );
    }
    else if ( mod == Qt::ControlModifier && (c == 'q' || c == 'w') ) {
        emit tabCloseRequested( currentIndex() );
    }
    else {
        QTabWidget::keyPressEvent( event );
    }
}
