/*
 * Copyright (C) 2009, 2010 Nicolas Bonnefon and other contributors
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

#ifndef MANUACTIONTOOLTIPBEHAVIOR_H
#define MANUACTIONTOOLTIPBEHAVIOR_H

#include <QObject>
#include <QPoint>

class QAction;
class QMenu;
class QTimerEvent;


// Provides a behavior to show an action's tooltip after mouse is unmoved for
// a specified number of 'ms'. E.g. used for tooltips with full-path for recent
// files in the file menu. Not thread-safe.
class MenuActionToolTipBehavior : public QObject
{
    Q_OBJECT;

 public:
    MenuActionToolTipBehavior(QAction *action, QMenu *parentMenu,
                              QObject *parent);

    // Time in ms that mouse needs to stay unmoved for tooltip to be shown
    int toolTipDelay(); /* ms */
    void setToolTipDelay(int ms);

 private:
    void timerEvent(QTimerEvent *event);
    void showToolTip(const QPoint &position);

 private slots:
    void onActionHovered();

 private:
    QAction *action;
    QMenu *parentMenu;
    int toolTipDelayMs;
    int timerId;
    QPoint hoverPoint;
};

#endif
