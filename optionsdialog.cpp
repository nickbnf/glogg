/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
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

#include <QtGui>

#include "log.h"

#include "optionsdialog.h"

// Constructor
OptionsDialog::OptionsDialog(QWidget* parent) : QDialog(parent)
{
    QGroupBox* fontBox  = new QGroupBox( tr("Font") );

    QLabel* sizeLabel   = new QLabel( tr("Size: ") );
    QLabel* familyLabel = new QLabel( tr("Family: ") );

    setupFontList();

    buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok |
            QDialogButtonBox::Cancel | QDialogButtonBox::Apply );

    connect(buttonBox, SIGNAL( clicked( QAbstractButton* ) ),
            this, SLOT( onButtonBoxClicked( QAbstractButton* ) ) );
    connect(fontFamilyBox, SIGNAL( currentIndexChanged(const QString& ) ),
            this, SLOT( updateFontSize( const QString& ) ));

    QHBoxLayout* fontLayout = new QHBoxLayout;
    fontLayout->addWidget(familyLabel);
    fontLayout->addWidget(fontFamilyBox);
    fontLayout->addWidget(sizeLabel);
    fontLayout->addWidget(fontSizeBox);
    fontBox->setLayout(fontLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(fontBox);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
    setWindowTitle( tr("Options") );
    setFixedHeight( sizeHint().height() );

    updateDialogFromConfig();
}

//
// Private functions
//

// Creates the two font ComboBoxes and populates the 'family' one
void OptionsDialog::setupFontList()
{
    QFontDatabase database;

    fontFamilyBox = new QComboBox();
    fontFamilyBox->addItems(database.families());

    fontSizeBox = new QComboBox();
}

// Updates the dialog box using values in global Config()
void OptionsDialog::updateDialogFromConfig()
{
    // Main font
    QFontInfo fontInfo = QFontInfo( Config().mainFont() );

    int familyIndex = fontFamilyBox->findText( fontInfo.family() );
    if ( familyIndex != -1 )
        fontFamilyBox->setCurrentIndex( familyIndex );

    int sizeIndex = fontSizeBox->findText( QString::number(fontInfo.pointSize()) );
    if ( sizeIndex != -1 )
        fontSizeBox->setCurrentIndex( sizeIndex );
}

//
// Slots
//

void OptionsDialog::updateFontSize(const QString& fontFamily)
{
    QFontDatabase database;
    QString oldFontSize = fontSizeBox->currentText();
    QList<int> sizes = database.pointSizes( fontFamily, "" );

    fontSizeBox->clear();
    foreach (int size, sizes) {
        fontSizeBox->addItem( QString::number(size) );
    }
    // Now restore the size we had before
    int i = fontSizeBox->findText(oldFontSize);
    if ( i != -1 ) 
        fontSizeBox->setCurrentIndex(i);
}

void OptionsDialog::updateConfigFromDialog()
{
    QFont font = QFont(
            fontFamilyBox->currentText(),
            (fontSizeBox->currentText()).toInt() );
    Config().setMainFont(font);

    emit optionsChanged();
}

void OptionsDialog::onButtonBoxClicked( QAbstractButton* button )
{
    QDialogButtonBox::ButtonRole role = buttonBox->buttonRole( button );
    if (   ( role == QDialogButtonBox::AcceptRole )
        || ( role == QDialogButtonBox::ApplyRole ) ) {
        updateConfigFromDialog();
    }

    if ( role == QDialogButtonBox::AcceptRole )
        accept();
    else if ( role == QDialogButtonBox::RejectRole )
        reject();
}
