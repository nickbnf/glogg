/*
 * Copyright (C) 2010 Nicolas Bonnefon and other contributors
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

class QuickFindWidget : public QWidget
{
  Q_OBJECT

  public:
    QuickFindWidget( QWidget* parent = 0 );

  private slots:
    void doSearchForward();
    void doSearchBackward();

  private:
    QHBoxLayout* layout_;

    QToolButton* closeButton_;
    QToolButton* nextButton_;
    QToolButton* previousButton_;
    QLineEdit*   editQuickFind_;

    QToolButton* setupToolButton(const QString &text, const QString &icon);
};

#endif
