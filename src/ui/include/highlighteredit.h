/*
 * Copyright (C) 2019 Anton Filimonov and other contributors
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

#ifndef KLOGG_HIGHLIGHTEREDIT_H
#define KLOGG_HIGHLIGHTEREDIT_H

#include <QWidget>

#include "highlighterset.h"
#include "ui_highlighteredit.h"

class HighlighterEdit : public QWidget, public Ui::HighlighterEdit {
    Q_OBJECT

  public:
    HighlighterEdit( Highlighter defaultHighlighter, QWidget* parent = nullptr );

    Highlighter highlighter() const;
    void setHighlighter( Highlighter highlighter );
    void reset();

  signals:
    void changed();

  private slots:
    void changeForeColor();
    void changeBackColor();
    void setPattern( const QString& pattern );
    void setIgnoreCase( bool ignoreCase );
    void setHighlightOnlyMatch( bool onlyMatch );

  private:
    void updateIcon( QPushButton* button, const QColor& color );
    static bool showColorPicker( const QColor& in, QColor& out );

  private:
    const Highlighter defaultHighlighter_;

    Highlighter highlighter_;
};

#endif // KLOGG_HIGHLIGHTEREDIT_H
