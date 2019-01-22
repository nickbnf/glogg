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

#include "versionchecker.h"

#include "persistentinfo.h"

#include "log.h"

#if defined(_WIN64)
#  define GLOGG_OS "win64"
#elif defined(_WIN32)
#  define GLOGG_OS "win32"
#elif defined(__APPLE__)
#  define GLOGG_OS "OSX"
#elif defined(__linux__)
#  define GLOGG_OS "linux"
#else
#  define GLOGG_OS "other"
#endif

const char* VersionChecker::VERSION_URL =
    "http://gloggversion.bonnefon.org/latest";

const uint64_t VersionChecker::CHECK_INTERVAL_S =
    3600 * 24 * 7; /* 7 days */

namespace {
    bool isVersionNewer( const QString& current, const QString& new_version );
};

VersionCheckerConfig::VersionCheckerConfig()
{
    enabled_ = true;
    next_deadline_ = 0;
}

void VersionCheckerConfig::retrieveFromStorage( QSettings& settings )
{
    if ( settings.contains( "versionchecker.enabled" ) )
        enabled_ = settings.value( "versionchecker.enabled" ).toBool();
    if ( settings.contains( "versionchecker.nextDeadline" ) )
        next_deadline_ = settings.value( "versionchecker.nextDeadline" ).toLongLong();
}

void VersionCheckerConfig::saveToStorage( QSettings& settings ) const
{
    settings.setValue( "versionchecker.enabled", enabled_ );
    settings.setValue( "versionchecker.nextDeadline",
            static_cast<long long>( next_deadline_ ) );
}

VersionChecker::VersionChecker() : QObject(), manager_( this )
{
}

VersionChecker::~VersionChecker()
{
}

void VersionChecker::startCheck()
{
    LOG(logDEBUG) << "VersionChecker::startCheck()";

    GetPersistentInfo().retrieve( "versionChecker" );

    auto config = Persistent<VersionCheckerConfig>( "versionChecker" );

    if ( config->versionCheckingEnabled() )
    {
        // Check the deadline has been reached
        if ( config->nextDeadline() < std::time( nullptr ) )
        {
            connect( &manager_, SIGNAL( finished( QNetworkReply* ) ),
                    this, SLOT( downloadFinished( QNetworkReply* ) ) );

            QNetworkRequest request;
            request.setUrl( QUrl( VERSION_URL ) );
            request.setRawHeader( "User-Agent", "glogg-" GLOGG_VERSION " (" GLOGG_OS ")" );
            manager_.get( request );
        }
        else
        {
            LOG(logDEBUG) << "Deadline not reached yet, next check in "
                << std::difftime( config->nextDeadline(), std::time( nullptr ) );
        }
    }
}

void VersionChecker::downloadFinished( QNetworkReply* reply )
{
    LOG(logDEBUG) << "VersionChecker::downloadFinished()";

    if ( reply->error() == QNetworkReply::NoError )
    {
        QString new_version = QString( reply->read( 256 ) ).remove( '\n' );

        LOG(logDEBUG) << "Latest version is " << new_version.toStdString();
        if ( isVersionNewer( QString( GLOGG_VERSION ), new_version ) )
        {
            LOG(logDEBUG) << "Sending new version notification";
            emit newVersionFound( new_version );
        }
    }
    else
    {
        LOG(logWARNING) << "Download failed: err " << reply->error();
    }

    reply->deleteLater();

    // Extend the deadline
    auto config = Persistent<VersionCheckerConfig>( "versionChecker" );

    config->setNextDeadline( std::time( nullptr ) + CHECK_INTERVAL_S );

    GetPersistentInfo().save( "versionChecker" );
}

namespace {
    bool isVersionNewer( const QString& current, const QString& new_version )
    {
        QRegularExpression version_regex( "(\\d+)\\.(\\d+)\\.(\\d+)(-(\\S+))?" );

        // Main version is the three first digits
        // Add is the part after '-' if there
        unsigned current_main_version = 0;
        unsigned current_add_version = 0;
        unsigned new_main_version = 0;
        unsigned new_add_version = 0;

        QRegularExpressionMatch currentMatch = version_regex.match( current );
        if ( currentMatch.hasMatch() )
        {
            current_main_version = currentMatch.captured(3).toInt()
                + currentMatch.captured(2).toInt() * 100
                + currentMatch.captured(1).toInt() * 10000;
            current_add_version = currentMatch.captured(5).isEmpty() ? 0 : 1;
        }

        QRegularExpressionMatch newMatch = version_regex.match( new_version );
        if ( newMatch.hasMatch() )
        {
            new_main_version = newMatch.captured(3).toInt()
                + newMatch.captured(2).toInt() * 100
                + newMatch.captured(1).toInt() * 10000;
            new_add_version = newMatch.captured(5).isEmpty() ? 0 : 1;
        }

        LOG(logDEBUG) << "Current version: " << current_main_version;
        LOG(logDEBUG) << "New version: " << new_main_version;

        // We only consider the main part for testing for now
        return new_main_version > current_main_version;
    }
};
