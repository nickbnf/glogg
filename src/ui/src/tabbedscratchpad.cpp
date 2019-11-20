/*
 * Copyright (C) 2019 Anton Filimonov and other contributors
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

#include "tabbedscratchpad.h"
#include "scratchpad.h"

#include <QVBoxLayout>
#include <memory>

TabbedScratchPad::TabbedScratchPad( QWidget* parent )
    : QWidget( parent )
{
    this->hide();

    auto tabWidget = std::make_unique<QTabWidget>();
    tabWidget_ = tabWidget.get();

    tabWidget_->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    tabWidget_->setTabsClosable( true );
    tabWidget_->setMovable( true );

    connect( tabWidget_, &QTabWidget::tabCloseRequested,
             [this]( const auto index ) { tabWidget_->removeTab( index ); } );

    addTab();

    auto layout = std::make_unique<QVBoxLayout>();
    layout->addWidget( tabWidget.release() );
    this->setLayout( layout.release() );
}

void TabbedScratchPad::keyPressEvent( QKeyEvent* event )
{
    const auto mod = event->modifiers();
    const auto key = event->key();

    event->accept();

    // Ctrl + tab
    if ( ( mod == Qt::ControlModifier && key == Qt::Key_Tab )
         || ( mod == Qt::ControlModifier && key == Qt::Key_PageDown )
         || ( mod == ( Qt::ControlModifier | Qt::AltModifier | Qt::KeypadModifier )
              && key == Qt::Key_Right ) ) {
        tabWidget_->setCurrentIndex( ( tabWidget_->currentIndex() + 1 ) % tabWidget_->count() );
    }
    // Ctrl + shift + tab
    else if ( ( mod == ( Qt::ControlModifier | Qt::ShiftModifier ) && key == Qt::Key_Tab )
              || ( mod == Qt::ControlModifier && key == Qt::Key_PageUp )
              || ( mod == ( Qt::ControlModifier | Qt::AltModifier | Qt::KeypadModifier )
                   && key == Qt::Key_Left ) ) {
        tabWidget_->setCurrentIndex( ( tabWidget_->currentIndex() - 1 >= 0 )
                                         ? tabWidget_->currentIndex() - 1
                                         : tabWidget_->count() - 1 );
    }
    else if ( mod == Qt::ControlModifier && ( key == Qt::Key_N ) ) {
        addTab();
    }
    else if ( mod == Qt::ControlModifier && ( key == Qt::Key_Q || key == Qt::Key_W ) ) {
        tabWidget_->removeTab( tabWidget_->currentIndex() );
    }
    else {
        event->setAccepted( false );
    }
}

void TabbedScratchPad::addTab()
{
    const auto newIndex
        = tabWidget_->addTab( new ScratchPad(), QString( "Scratchpad %1" ).arg( ++tabCounter_ ) );
    tabWidget_->setCurrentIndex( newIndex );
}
