/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov and other contributors
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

#ifndef KLOGG_KLOGGAPP_H
#define KLOGG_KLOGGAPP_H

#include <QApplication>

#if QT_VERSION >= QT_VERSION_CHECK( 5, 12, 0 )
#include <QCborValue>
#endif

#include <QDir>
#include <QMessageBox>
#include <QNetworkProxyFactory>
#include <QUuid>

#ifdef Q_OS_MAC
#include <QFileOpenEvent>
#endif

#include <stack>

#include "configuration.h"
#include "klogg_version.h"
#include "log.h"
#include "session.h"
#include "uuid.h"

#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>

#include <singleapp/singleapplication.h>

#include "crash_tracer.h"
#include "mainwindow.h"
#include "messagereceiver.h"
#include "versionchecker.h"

class KloggApp : public SingleApplication {

    Q_OBJECT

  public:
    KloggApp( int& argc, char* argv[] )
        : SingleApplication( argc, argv, true,
                             SingleApplication::SecondaryNotification
                                 | SingleApplication::ExcludeAppPath
                                 | SingleApplication::ExcludeAppVersion )
    {
        crashTracer_ = std::make_unique<CrashTracer>( argv[ 0 ] );

        QNetworkProxyFactory::setUseSystemConfiguration( true );

        qRegisterMetaType<LoadingStatus>( "LoadingStatus" );
        qRegisterMetaType<LinesCount>( "LinesCount" );
        qRegisterMetaType<LineNumber>( "LineNumber" );
        qRegisterMetaType<std::vector<LineNumber>>( "std::vector<LineNumber>" );
        qRegisterMetaType<LineLength>( "LineLength" );
        qRegisterMetaType<Portion>( "Portion" );
        qRegisterMetaType<Selection>( "Selection" );
        qRegisterMetaType<QFNotification>( "QFNotification" );
        qRegisterMetaType<QFNotificationReachedEndOfFile>( "QFNotificationReachedEndOfFile" );
        qRegisterMetaType<QFNotificationReachedBegininningOfFile>(
            "QFNotificationReachedBegininningOfFile" );
        qRegisterMetaType<QFNotificationProgress>( "QFNotificationProgress" );
        qRegisterMetaType<QFNotificationInterrupted>( "QFNotificationInterrupted" );
        qRegisterMetaType<QuickFindMatcher>( "QuickFindMatcher" );

        if ( isPrimary() ) {
            QObject::connect( this, &SingleApplication::receivedMessage, &messageReceiver_,
                              &MessageReceiver::receiveMessage, Qt::QueuedConnection );

            QObject::connect( &messageReceiver_, &MessageReceiver::loadFile, this,
                              &KloggApp::loadFileNonInteractive );

            // Version checker notification
            connect( &versionChecker_, &VersionChecker::newVersionFound,
                     [this]( const QString& new_version, const QString& url ) {
                         newVersionNotification( new_version, url );
                     } );
        }
    }

    void sendFilesToPrimaryInstance( const std::vector<QString>& filenames )
    {
#ifdef Q_OS_WIN
        ::AllowSetForegroundWindow( static_cast<DWORD>( primaryPid() ) );
#endif

        QTimer::singleShot( 100, [&filenames, this] {
            QStringList filesToOpen;

            for ( const auto& filename : filenames ) {
                filesToOpen.append( filename );
            }

            QVariantMap data;
            data.insert( "version", kloggVersion() );
            data.insert( "files", QVariant{ filesToOpen } );

#if QT_VERSION >= QT_VERSION_CHECK( 5, 12, 0 )
            auto cbor = QCborValue::fromVariant( data );
            this->sendMessage( cbor.toCbor(), 5000 );
#else
            auto json = QJsonDocument::fromVariant( data );
            this->sendMessage( json.toBinaryData(), 5000 );
#endif

            QTimer::singleShot( 100, this, &SingleApplication::quit );
        } );
    }

    void initLogger( plog::Severity log_level, bool log_to_file )
    {
        QString log_file_name
            = QString( "klogg_%1_%2.log" )
                  .arg( QDateTime::currentDateTime().toString( "yyyy-MM-dd_HH-mm-ss" ) )
                  .arg( applicationPid() );

        tempAppender_ = std::make_unique<plog::RollingFileAppender<plog::GloggFormatter>>(
            QDir::temp().filePath( log_file_name ).toStdString().c_str(), 10 * 1024 * 1024, 5 );

        plog::init<1>( plog::none, tempAppender_.get() );

        if ( log_to_file ) {
            logAppender_ = std::make_unique<plog::RollingFileAppender<plog::GloggFormatter>>(
                log_file_name.toStdString().c_str() );
        }
        else {
            logAppender_ = std::make_unique<plog::ColorConsoleAppender<plog::GloggFormatter>>();
        }

        plog::init( log_level, logAppender_.get() ).addAppender( plog::get<1>() );
    }

    MainWindow* reloadSession()
    {
        if ( !session_ ) {
            session_ = std::make_shared<Session>();
        }

        for ( auto&& windowSession : session_->windowSessions() ) {
            auto w = newWindow( std::move( windowSession ) );
            w->reloadGeometry();
            w->reloadSession();
            w->show();
        }

        if ( mainWindows_.empty() ) {
            auto w = newWindow();
            w->show();
        }

        return mainWindows_.back().second;
    }

    MainWindow* newWindow()
    {
        if ( !session_ ) {
            session_ = std::make_shared<Session>();
        }

        return newWindow( { session_, generateIdFromUuid(), nextWindowIndex() } );
    }

    void loadFileNonInteractive( const QString& file )
    {
        while ( !activeWindows_.empty() && activeWindows_.top().isNull() ) {
            activeWindows_.pop();
        }

        if ( activeWindows_.empty() ) {
            newWindow();
        }

        activeWindows_.top()->loadFileNonInteractive( file );
    }

    void startBackgroundTasks()
    {
        LOG( logDEBUG ) << "startBackgroundTasks";
        versionChecker_.startCheck();
    }

#ifdef Q_OS_MAC
    bool event( QEvent* event ) override
    {
        if ( event->type() == QEvent::FileOpen ) {
            QFileOpenEvent* openEvent = static_cast<QFileOpenEvent*>( event );
            LOG( logINFO ) << "File open request " << openEvent->file();

            if ( isPrimary() ) {
                loadFileNonInteractive( openEvent->file() );
            }
            else {
                sendFilesToPrimaryInstance( { openEvent->file() } );
            }
        }

        return SingleApplication::event( event );
    }
#endif

  private:
    MainWindow* newWindow( WindowSession&& session )
    {
        mainWindows_.emplace_back( session, new MainWindow( session ) );

        auto& window = mainWindows_.back().second;

        activeWindows_.push( QPointer<MainWindow>( window ) );

        LOG( logINFO ) << "Window " << &window << " created";
        connect( window, &MainWindow::newWindow, [=]() { newWindow()->show(); } );
        connect( window, &MainWindow::windowActivated,
                 [this, window]() { onWindowActivated( *window ); } );
        connect( window, &MainWindow::windowClosed,
                 [this, window]() { onWindowClosed( *window ); } );
        connect( window, &MainWindow::exitRequested, [this] { exitApplication(); } );

        return window;
    }

    void onWindowActivated( MainWindow& window )
    {
        LOG( logINFO ) << "Window " << &window << " activated";
        activeWindows_.push( QPointer<MainWindow>( &window ) );
    }

    void onWindowClosed( MainWindow& window )
    {
        LOG( logINFO ) << "Window " << &window << " closed";
        auto w = std::find_if( mainWindows_.begin(), mainWindows_.end(),
                               [&window]( const auto& p ) { return p.second == &window; } );

        if ( w != mainWindows_.end() ) {
            mainWindows_.erase( w );
        }
    }

    void exitApplication()
    {
        LOG( logINFO ) << "exit application";
        session_->setExitRequested( true );
        auto mainWindows = mainWindows_;
        mainWindows.reverse();
        for ( auto window : mainWindows ) {
            window.second->close();
        }

        QTimer::singleShot( 100, this, &QCoreApplication::quit );
    }

    void newVersionNotification( const QString& new_version, const QString& url )
    {
        LOG( logDEBUG ) << "newVersionNotification( " << new_version << " from " << url << " )";

        QMessageBox msgBox;
        msgBox.setText( QString( "A new version of klogg (%1) is available for download <p>"
                                 "<a href=\"%2\">%2</a>" )
                            .arg( new_version, url ) );
        msgBox.exec();
    }

    size_t nextWindowIndex() const
    {
        if ( mainWindows_.empty() ) {
            return 0;
        }
        else {
            return size_t{ 1 }
                   + std::accumulate( mainWindows_.begin(), mainWindows_.end(), size_t{},
                                      []( size_t current, const auto& next ) {
                                          return std::max( current, next.first.windowIndex() );
                                      } );
        }
    }

  private:
    std::unique_ptr<plog::RollingFileAppender<plog::GloggFormatter>> tempAppender_;
    std::unique_ptr<plog::IAppender> logAppender_;

    MessageReceiver messageReceiver_;

    std::shared_ptr<Session> session_;

    std::list<std::pair<WindowSession, MainWindow*>> mainWindows_;
    std::stack<QPointer<MainWindow>> activeWindows_;

    VersionChecker versionChecker_;

    std::unique_ptr<CrashTracer> crashTracer_;
};

#endif // KLOGG_KLOGGAPP_H
