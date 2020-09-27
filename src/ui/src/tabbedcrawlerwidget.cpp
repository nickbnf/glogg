/*
 * Copyright (C) 2014, 2015 Nicolas Bonnefon and other contributors
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

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>

#include "crawlerwidget.h"

#include "iconloader.h"
#include "log.h"
#include "openfilehelper.h"
#include "tabnamemapping.h"

namespace {
constexpr QLatin1String PathKey = QLatin1String( "path", 4 );
constexpr QLatin1String StatusKey = QLatin1String( "status", 6 );
} // namespace

TabbedCrawlerWidget::TabbedCrawlerWidget()
    : QTabWidget()
    , newdata_icon_( ":/images/newdata_icon.png" )
    , newfiltered_icon_( ":/images/newfiltered_icon.png" )
    , myTabBar_()
{

#ifdef Q_OS_WIN
    myTabBar_.setStyleSheet( "QTabBar::tab {\
            height: 20px; "
                             "} "
                             "QTabBar::close-button {\
              height: 6px; width: 6px;\
              subcontrol-origin: padding;\
              subcontrol-position: left;\
             }" );
#else
    // On GTK style, it looks better with a smaller font
    myTabBar_.setStyleSheet( "QTabBar::tab {"
                             " height: 20px; "
                             " font-size: 9pt; "
                             "} "
                             "QTabBar::close-button {\
              height: 6px; width: 6px;\
              subcontrol-origin: padding;\
              subcontrol-position: left;\
             }" );
#endif
    setTabBar( &myTabBar_ );
    myTabBar_.hide();

    myTabBar_.setContextMenuPolicy( Qt::CustomContextMenu );
    connect( &myTabBar_, &QWidget::customContextMenuRequested, this,
             &TabbedCrawlerWidget::showContextMenu );

    QTimer::singleShot( 0, [this] { loadIcons(); } );
}

void TabbedCrawlerWidget::loadIcons()
{
    IconLoader iconLoader{ this };

    olddata_icon_ = iconLoader.load( "olddata_icon" );
    for ( int tab = 0; tab < count(); ++tab ) {
        updateIcon( tab );
    }
}

void TabbedCrawlerWidget::changeEvent( QEvent* event )
{
    if ( event->type() == QEvent::StyleChange ) {
        QTimer::singleShot( 0, [this] { loadIcons(); } );
    }

    QWidget::changeEvent( event );
}

void TabbedCrawlerWidget::addTabBarItem( int index, const QString& file_name )
{
    const auto tab_label = QFileInfo( file_name ).fileName();
    const auto tabName = TabNameMapping::getSynced().tabName( file_name );

    myTabBar_.setTabText( index, tabName.isEmpty() ? tab_label : tabName );
    myTabBar_.setTabToolTip( index, QDir::toNativeSeparators( file_name ) );

    QVariantMap tabData;
    tabData[ PathKey ] = file_name;
    tabData[ StatusKey ] = static_cast<int>( DataStatus::OLD_DATA );

    myTabBar_.setTabData( index, tabData );

    // Display the icon
    auto icon_label = std::make_unique<QLabel>();
    icon_label->setPixmap( olddata_icon_.pixmap( 11, 12 ) );
    icon_label->setAlignment( Qt::AlignCenter );
    myTabBar_.setTabButton( index, QTabBar::RightSide, icon_label.release() );

    LOG( logDEBUG ) << "addTab, count = " << count();
    LOG( logDEBUG ) << "width = " << olddata_icon_.pixmap( 11, 12 ).devicePixelRatio();

    if ( count() > 1 )
        myTabBar_.show();
}

void TabbedCrawlerWidget::removeCrawler( int index )
{
    QTabWidget::removeTab( index );

    if ( count() <= 1 )
        myTabBar_.hide();
}

void TabbedCrawlerWidget::mouseReleaseEvent( QMouseEvent* event )
{
    LOG( logDEBUG ) << "TabbedCrawlerWidget::mouseReleaseEvent";

    if ( event->button() == Qt::MidButton ) {
        int tab = this->myTabBar_.tabAt( event->pos() );
        if ( -1 != tab ) {
            emit tabCloseRequested( tab );
            event->accept();
        }
    }

    event->ignore();
}

QString TabbedCrawlerWidget::tabPathAt( int index ) const
{
    return myTabBar_.tabData( index ).toMap()[ PathKey ].toString();
}

void TabbedCrawlerWidget::showContextMenu( const QPoint& point )
{
    int tab = myTabBar_.tabAt( point );
    if ( -1 != tab ) {
        QMenu menu( this );
        auto closeThis = menu.addAction( "Close this" );
        auto closeOthers = menu.addAction( "Close others" );
        auto closeLeft = menu.addAction( "Close to the left" );
        auto closeRight = menu.addAction( "Close to the right" );
        auto closeAll = menu.addAction( "Close all" );
        menu.addSeparator();
        auto copyFullPath = menu.addAction( "Copy full path" );
        auto openContainingFolder = menu.addAction( "Open containing folder" );
        menu.addSeparator();
        auto renameTab = menu.addAction( "Rename tab" );
        auto resetTabName = menu.addAction( "Reset tab name" );

        connect( closeThis, &QAction::triggered, [tab, this] { emit tabCloseRequested( tab ); } );

        connect( closeOthers, &QAction::triggered, [tabWidget = widget( tab ), this] {
            while ( count() != 1 ) {
                for ( int i = 0; i < count(); ++i ) {
                    if ( i != indexOf( tabWidget ) ) {
                        emit tabCloseRequested( i );
                        break;
                    }
                }
            }
        } );

        connect( closeLeft, &QAction::triggered, [tabWidget = widget( tab ), this] {
            while ( indexOf( tabWidget ) != 0 ) {
                emit tabCloseRequested( 0 );
            }
        } );

        connect( closeRight, &QAction::triggered, [tab, this] {
            while ( count() > tab + 1 ) {
                emit tabCloseRequested( tab + 1 );
            }
        } );

        connect( closeAll, &QAction::triggered, [this] {
            while ( count() ) {
                emit tabCloseRequested( 0 );
            }
        } );

        if ( tab == 0 ) {
            closeLeft->setDisabled( true );
        }
        else if ( tab == count() - 1 ) {
            closeRight->setDisabled( true );
        }

        connect( copyFullPath, &QAction::triggered,
                 [this, tab] { QApplication::clipboard()->setText( tabToolTip( tab ) ); } );

        connect( openContainingFolder, &QAction::triggered,
                 [this, tab] { showPathInFileExplorer( tabToolTip( tab ) ); } );

        connect( renameTab, &QAction::triggered, [this, tab] {
            bool isNameEntered = false;
            auto newName = QInputDialog::getText( this, "Rename tab", "Tab name", QLineEdit::Normal,
                                                  myTabBar_.tabText( tab ), &isNameEntered );
            if ( isNameEntered ) {
                const auto tabPath = tabPathAt( tab );
                TabNameMapping::getSynced().setTabName( tabPath, newName ).save();

                if ( newName.isEmpty() ) {
                    myTabBar_.setTabText( tab, QFileInfo( tabPath ).fileName() );
                }
                else {
                    myTabBar_.setTabText( tab, std::move( newName ) );
                }
            }
        } );

        connect( resetTabName, &QAction::triggered, [this, tab] {
            const auto tabPath = tabPathAt( tab );
            TabNameMapping::getSynced().setTabName( tabPath, "" ).save();
            myTabBar_.setTabText( tab, QFileInfo( tabPath ).fileName() );
        } );

        menu.exec( myTabBar_.mapToGlobal( point ) );
    }
}

void TabbedCrawlerWidget::keyPressEvent( QKeyEvent* event )
{
    const auto mod = event->modifiers();
    const auto key = event->key();

    LOG( logDEBUG ) << "TabbedCrawlerWidget::keyPressEvent";

    // Ctrl + tab
    if ( ( mod == Qt::ControlModifier && key == Qt::Key_Tab )
         || ( mod == Qt::ControlModifier && key == Qt::Key_PageDown )
         || ( mod == ( Qt::ControlModifier | Qt::AltModifier | Qt::KeypadModifier )
              && key == Qt::Key_Right ) ) {
        setCurrentIndex( ( currentIndex() + 1 ) % count() );
    }
    // Ctrl + shift + tab
    else if ( ( mod == ( Qt::ControlModifier | Qt::ShiftModifier ) && key == Qt::Key_Tab )
              || ( mod == Qt::ControlModifier && key == Qt::Key_PageUp )
              || ( mod == ( Qt::ControlModifier | Qt::AltModifier | Qt::KeypadModifier )
                   && key == Qt::Key_Left ) ) {
        setCurrentIndex( ( currentIndex() - 1 >= 0 ) ? currentIndex() - 1 : count() - 1 );
    }
    // Ctrl + numbers
    else if ( mod == Qt::ControlModifier && ( key >= Qt::Key_1 && key <= Qt::Key_8 ) ) {
        int new_index = key - Qt::Key_0;
        if ( new_index <= count() )
            setCurrentIndex( new_index - 1 );
    }
    // Ctrl + 9
    else if ( mod == Qt::ControlModifier && key == Qt::Key_9 ) {
        setCurrentIndex( count() - 1 );
    }
    else if ( mod == Qt::ControlModifier && ( key == Qt::Key_Q || key == Qt::Key_W ) ) {
        emit tabCloseRequested( currentIndex() );
    }
    else {
        QTabWidget::keyPressEvent( event );
    }
}

void TabbedCrawlerWidget::updateIcon( int index )
{
    auto tabData = myTabBar_.tabData( index ).toMap();
    auto* icon_label = qobject_cast<QLabel*>( myTabBar_.tabButton( index, QTabBar::RightSide ) );

    if ( icon_label ) {
        const QIcon* icon;
        switch ( static_cast<DataStatus>( tabData[ StatusKey ].toInt() ) ) {
        case DataStatus::OLD_DATA:
            icon = &olddata_icon_;
            break;
        case DataStatus::NEW_DATA:
            icon = &newdata_icon_;
            break;
        case DataStatus::NEW_FILTERED_DATA:
            icon = &newfiltered_icon_;
            break;
        default:
            return;
        }

        icon_label->setPixmap( icon->pixmap( 12, 12 ) );
    }
}

void TabbedCrawlerWidget::setTabDataStatus( int index, DataStatus status )
{
    LOG( logDEBUG ) << "TabbedCrawlerWidget::setTabDataStatus " << index;

    auto tabData = myTabBar_.tabData( index ).toMap();
    tabData[ StatusKey ] = static_cast<int>( status );
    myTabBar_.setTabData( index, tabData );

    updateIcon( index );
}
