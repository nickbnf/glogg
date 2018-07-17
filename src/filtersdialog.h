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

#include "filterset.h"
#include "ui_filtersdialog.h"

class FiltersDialog : public QDialog, public Ui::FiltersDialog
{
  Q_OBJECT

  public:
    FiltersDialog( QWidget* parent = 0 );

  signals:
    // Is emitted when new settings must be used
    void optionsChanged();

  private slots:
    void on_addFilterButton_clicked();
    void on_removeFilterButton_clicked();
    void on_buttonBox_clicked( QAbstractButton* button );
    void on_upFilterButton_clicked();
    void on_downFilterButton_clicked();
    void on_ignoreCaseCheckBox_clicked();
    void on_patternEdit_editingFinished();
    void on_foreColorButton_clicked();
    void on_backColorButton_clicked();
    // Update the property (pattern, color...) fields from the
    // selected Filter.
    void updatePropertyFields();


  private:
    static bool showColorPicker(const QColor& in , QColor& out);
    void updateIcon(QPushButton* button , QColor color);
    // Temporary filterset modified by the dialog
    // it is copied from the one in Config()
    std::shared_ptr<FilterSet> filterSet;
    QColor foreColor , backColor;

    // Index of the row currently selected or -1 if none.
    int selectedRow_;

    void populateColors();
    void populateFilterList();

};

#endif
