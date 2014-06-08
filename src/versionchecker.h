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

#ifndef VERSIONCHECKER_H
#define VERSIONCHECKER_H

#include <QObject>
#include <QtNetwork>

// This class compares the current version number with the latest
// stored on a central server
class VersionChecker : public QObject
{
  Q_OBJECT

  public:
    VersionChecker();
    ~VersionChecker();

    // Starts an asynchronous check for a newer version if it is needed.
    // A newVersionFound signal is sent if one is found.
    // In case of error or if no new version is found, no signal is emitted.
    void startCheck();

  signals:
    // New version "version" is available
    void newVersionFound( const QString& version );

  private slots:
    // Called when download is finished
    void downloadFinished( QNetworkReply* );

  private:
    static const char* VERSION_URL;

    QNetworkAccessManager manager_;
};

#endif
