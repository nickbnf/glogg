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

#ifndef FILTERSDIALOG_H
#define FILTERSDIALOG_H

#include <memory>

#include <QDialog>

#include "highlighterset.h"
#include "highlightersetedit.h"
#include "ui_highlightersdialog.h"

class HighlightersDialog : public QDialog, public Ui::HighlightersDialog {
    Q_OBJECT

  public:
    explicit HighlightersDialog( QWidget* parent = nullptr );

  signals:
    // Is emitted when new settings must be used
    void optionsChanged();

  private slots:
    void addHighlighterSet();
    void removeHighlighterSet();

    void moveHighlighterSetUp();
    void moveHighlighterSetDown();

    void resolveDialog( QAbstractButton* button );

    // Update the edit fields from the selected HighlighterSet.
    void updatePropertyFields();

    // Update the selected HighlighterSet from the values in the property fields.
    void updateHighlighterProperties();

    void exportHighlighters();
    void importHighlighters();

  private:
    void populateHighlighterList();
    void setCurrentRow( int row );
    void updateGroupTitle( const HighlighterSet& set );

  private:
    HighlighterSetEdit* highlighterSetEdit_;

    // Temporary HighlighterSetCollection modified by the dialog
    // it is copied from the one in Config()
    HighlighterSetCollection highlighterSetCollection_;

    // Index of the row currently selected or -1 if none.
    int selectedRow_;
};

#endif
