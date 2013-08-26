/*
 * Copyright (C) 2013 Nicolas Bonnefon and other contributors
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

#ifndef QFNOTIFICATIONS_H
#define QFNOTIFICATIONS_H

#include <QObject>
#include <QWidget>
#include <QFontMetrics>

// Notifications sent by the QF for displaying to the user
// and their translation in UI text.
class QFNotification {
  public:
    virtual QString message() const = 0;

    // Max width of the message (in pixels)
    static int maxWidth( const QWidget* widget ) {
        QFontMetrics fm = widget->fontMetrics();
        return qMax( fm.size( Qt::TextSingleLine, REACHED_BOF ).width(),
                     fm.size( Qt::TextSingleLine, REACHED_EOF ).width() );
    }

  protected:
    static const QString REACHED_EOF;
    static const QString REACHED_BOF;
};

class QFNotificationReachedEndOfFile : public QFNotification
{
    QString message() const {
        return REACHED_EOF;
    }
};

class QFNotificationReachedBegininningOfFile : public QFNotification
{
    QString message() const {
        return REACHED_BOF;
    }
};

class QFNotificationProgress : public QFNotification {
  public:
    // Constructor taking the progress (in percent)
    QFNotificationProgress( int progress_percent )
    { progressPercent_ = progress_percent; }

    QString message() const {
        return QString( QObject::tr("Searching (position %1 %)")
                .arg( progressPercent_ ) );
    }
  private:
    int progressPercent_;
};

#endif
