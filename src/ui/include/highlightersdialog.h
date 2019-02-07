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

#ifndef FILTERSDIALOG_H
#define FILTERSDIALOG_H

#include <memory>

#include <QDialog>

#include "highlighterset.h"
#include "ui_highlightersdialog.h"

class HighlightersDialog : public QDialog, public Ui::HighlightersDialog
{
  Q_OBJECT

  public:
    HighlightersDialog( QWidget* parent = nullptr );

  signals:
    // Is emitted when new settings must be used
    void optionsChanged();

  private slots:
    void on_addHighlighterButton_clicked();
    void on_removeHighlighterButton_clicked();
    void on_buttonBox_clicked( QAbstractButton* button );
    void on_upHighlighterButton_clicked();
    void on_downHighlighterButton_clicked();
    void on_foreColorButton_clicked();
    void on_backColorButton_clicked();
    // Update the property (pattern, color...) fields from the
    // selected Highlighter.
    void updatePropertyFields();
    // Update the selected Highlighter from the values in the property fields.
    void updateHighlighterProperties();

  private:
    // Temporary HighlighterSet modified by the dialog
    // it is copied from the one in Config()
    HighlighterSet highlighterSet_;

    // Index of the row currently selected or -1 if none.
    int selectedRow_;

    QColor foreColor_ , backColor_;

    static bool showColorPicker(const QColor& in , QColor& out);
    void updateIcon(QPushButton* button , QColor color);

    void populateHighlighterList();
};

#endif
