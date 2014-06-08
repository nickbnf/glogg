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

#include "log.h"

const char* VersionChecker::VERSION_URL =
    "http://gloggversion.bonnefon.org/latest";

VersionChecker::VersionChecker() : QObject(), manager_( this )
{
}

VersionChecker::~VersionChecker()
{
}

void VersionChecker::startCheck()
{
    LOG(logDEBUG) << "VersionChecker::startCheck()";

    connect( &manager_, SIGNAL( finished( QNetworkReply* ) ),
            this, SLOT( downloadFinished( QNetworkReply* ) ) );

    QNetworkRequest request;
    request.setUrl( QUrl( VERSION_URL ) );
    request.setRawHeader( "User-Agent", "glogg-" GLOGG_VERSION );
    manager_.get( request );
}

void VersionChecker::downloadFinished( QNetworkReply* reply )
{
    LOG(logDEBUG) << "VersionChecker::downloadFinished()";
}
