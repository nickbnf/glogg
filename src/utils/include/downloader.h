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

#ifndef KLOGG_DOWNLOADER_H
#define KLOGG_DOWNLOADER_H

#include <QFile>
#include <QNetworkAccessManager>

class Downloader : public QObject {
    Q_OBJECT
  public:
    explicit Downloader( QObject* parent = nullptr );

    void download( const QUrl& url, QFile* outputFile );

    QString lastError() const;

  signals:
    void downloadProgress( qint64 bytesReceived, qint64 bytesTotal );
    void finished( bool );

  private slots:
    void downloadFinished();
    void downloadReadyRead();

  private:
    QNetworkAccessManager manager_;
    QNetworkReply* currentDownload_ = nullptr;

    QString lastError_;

    QFile* output_;
};

#endif // KLOGG_DOWNLOADER_H
