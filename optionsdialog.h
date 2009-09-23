/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
 *
 * This file is part of LogCrawler.
 *
 * LogCrawler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LogCrawler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LogCrawler.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QComboBox>

#include "configuration.h"

// Implements the main option dialog box
class OptionsDialog : public QDialog
{
    Q_OBJECT

  public:
    OptionsDialog(QWidget* parent = 0);

  signals:
    // Is emitted when new settings must be used
    void optionsChanged();

  private slots:
    /*
    void applyClicked();
    */
    // Clears and updates the font size box with the sizes allowed
    // by the passed font family.
    void updateFontSize(const QString& fontFamily);
    // Update the content of the global Config() using parameters
    // from the dialog box.
    void updateConfigFromDialog();

  private:
    QPushButton* okButton;
    QPushButton* cancelButton;
    QPushButton* applyButton;

    QComboBox*   fontFamilyBox;
    QComboBox*   fontSizeBox;

    void setupFontList();
    void updateDialogFromConfig();
};

#endif
