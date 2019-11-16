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
#include <QDir>
#include <stack>

#include "log.h"
#include "session.h"

#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>

#include <singleapp/singleapplication.h>

#include "mainwindow.h"
#include "messagereceiver.h"

class KloggApp : public SingleApplication {

    Q_OBJECT

  public:
    KloggApp( int& argc, char* argv[] )
        : SingleApplication( argc, argv, true, SingleApplication::SecondaryNotification )
    {
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
        }
    }

    void sendFilesToPrimaryInstance( const std::vector<QString>& filenames )
    {
#ifdef Q_OS_WIN
        ::AllowSetForegroundWindow( primaryPid() );
#endif

        QTimer::singleShot( 100, [&filenames, this] {
            QStringList filesToOpen;

            for ( const auto& filename : filenames ) {
                filesToOpen.append( filename );
            }

            QVariantMap data;
            data.insert( "version", GLOGG_VERSION );
            data.insert( "files", QVariant{ filesToOpen } );

            auto json = QJsonDocument::fromVariant( data );
            this->sendMessage( json.toBinaryData(), 5000 );

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

    MainWindow* newWindow()
    {
        if ( !session_ ) {
            session_ = std::make_shared<Session>();
        }

        mainWindows_.emplace_back( new MainWindow(*session_) );

        auto window = mainWindows_.back();

        activeWindows_.push( QPointer<MainWindow>( window ) );

        LOG( logINFO ) << "Window " << &window << " created";
        connect( window, &MainWindow::newWindow, [=]() { newWindow()->show(); } );
        connect( window, &MainWindow::windowActivated,
                 [this, window]() { onWindowActivated( *window ); } );
        connect( window, &MainWindow::exitRequested,
                 [this] { QTimer::singleShot( 100, this, &QCoreApplication::quit ); } );

        return window;
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

  private:
    void onWindowActivated( MainWindow& window )
    {
        LOG( logINFO ) << "Window " << &window << " activated";
        activeWindows_.push( QPointer<MainWindow>( &window ) );
    }

  private:
    std::unique_ptr<plog::RollingFileAppender<plog::GloggFormatter>> tempAppender_;
    std::unique_ptr<plog::IAppender> logAppender_;

    MessageReceiver messageReceiver_;

    std::shared_ptr<Session> session_;

    std::list<MainWindow*> mainWindows_;
    std::stack<QPointer<MainWindow>> activeWindows_;
};

#endif // KLOGG_KLOGGAPP_H
