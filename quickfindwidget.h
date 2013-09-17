/*
 * Copyright (C) 2010, 2013 Nicolas Bonnefon and other contributors
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

#ifndef QUICKFINDWIDGET_H
#define QUICKFINDWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QLabel>
#include <QTimer>

class QFNotification;

enum QFDirection {
    Forward,
    Backward,
};

class QuickFindWidget : public QWidget
{
  Q_OBJECT

  public:
    QuickFindWidget( QWidget* parent = 0 );

    // Show the widget with the given direction
    // when requested by the user (the widget won't timeout)
    void userActivate();

  public slots:
    // Instructs the widget to change the pattern displayed
    void changeDisplayedPattern( const QString& newPattern );

    // Show the widget for a notification (will timeout)
    void notify( const QFNotification& message );
    // Clear the notification
    void clearNotification();

  private slots:
    void doSearchForward();
    void doSearchBackward();
    void returnHandler();
    void closeHandler();
    void notificationTimeout();

  signals:
    // Sent when Return is presed to confirm the pattern
    void patternConfirmed( const QString& );
    // Sent every time the pattern is modified
    void patternUpdated( const QString& );
    void close();
    void searchForward();
    void searchBackward();
    void searchNext();

  private:
    const static int NOTIFICATION_TIMEOUT;

    QHBoxLayout* layout_;

    QToolButton* closeButton_;
    QToolButton* nextButton_;
    QToolButton* previousButton_;
    QLineEdit*   editQuickFind_;
    QLabel*      notificationText_;

    QToolButton* setupToolButton(const QString &text, const QString &icon);

    QTimer*      notificationTimer_;

    QFDirection  direction_;

    // Whether the user explicitely wants us on the screen
    bool         userRequested_;
};

#endif
