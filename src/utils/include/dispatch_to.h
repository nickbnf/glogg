/*
 * Copyright (C) 2021 Anton Filimonov
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

#ifndef KLOGG_DISPATCH_TO

#include <QAbstractEventDispatcher>
#include <QApplication>
#include <QMetaObject>
#include <QThread>

#include "log.h"

// by
// https://github.com/KubaO/stackoverflown/blob/master/questions/metacall-21646467/main.cpp
#if QT_VERSION >= QT_VERSION_CHECK( 5, 10, 0 )
template <typename F>
static void dispatchToObject( F&& fun, QObject* obj = qApp )
{
    QMetaObject::invokeMethod( obj, std::forward<F>( fun ),  Qt::QueuedConnection );
}

template <typename F>
static void dispatchToThread( F&& fun, QThread* thread = qApp->thread() )
{
    auto* obj = QAbstractEventDispatcher::instance( thread );
    Q_ASSERT( obj );
    QMetaObject::invokeMethod( obj, std::forward<F>( fun ), Qt::QueuedConnection );
}
#else
namespace detail {
template <typename F>
struct FEvent : public QEvent {
    using Fun = typename std::decay<F>::type;

    FEvent( Fun&& fun )
        : QEvent( QEvent::None )
        , fun_( std::move( fun ) )
    {
    }
    FEvent( const Fun& fun )
        : QEvent( QEvent::None )
        , fun_( fun )
    {
    }
    ~FEvent()
    {
        fun_();
    }

  private:
    Fun fun_;
};
} // namespace detail

template <typename F>
static void dispatchToObject( F&& fun, QObject* obj = qApp )
{
    if ( qobject_cast<QThread*>( obj ) ) {
        LOG_WARNING << "posting a call to a thread object - consider using postToThread";
    }
    QCoreApplication::postEvent( obj, new detail::FEvent<F>( std::forward<F>( fun ) ) );
}

template <typename F>
static void dispatchToThread( F&& fun, QThread* thread = qApp->thread() )
{
    QObject* obj = QAbstractEventDispatcher::instance( thread );
    Q_ASSERT( obj );
    QCoreApplication::postEvent( obj, new detail::FEvent<F>( std::forward<F>( fun ) ) );
}
#endif

template <typename F>
static void dispatchToMainThread( F&& fun )
{
    dispatchToThread( std::forward<F>( fun ) );
}

#endif