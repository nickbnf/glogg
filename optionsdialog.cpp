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

#include <QtGui>

#include "log.h"

#include "optionsdialog.h"

// Constructor
OptionsDialog::OptionsDialog( QWidget* parent ) : QDialog(parent)
{
    setupUi( this );

    setupFontList();
    setupRegexp();

    connect(buttonBox, SIGNAL( clicked( QAbstractButton* ) ),
            this, SLOT( onButtonBoxClicked( QAbstractButton* ) ) );
    connect(fontFamilyBox, SIGNAL( currentIndexChanged(const QString& ) ),
            this, SLOT( updateFontSize( const QString& ) ));

    updateDialogFromConfig();
}

//
// Private functions
//

// Populates the 'family' ComboBox
void OptionsDialog::setupFontList()
{
    QFontDatabase database;

    fontFamilyBox->addItems(database.families());
}

// Populate the regexp ComboBoxes
void OptionsDialog::setupRegexp()
{
    QStringList regexpTypes;

    regexpTypes << tr("Extended Regexp")
        << tr("Wildcards") << tr("Fixed Strings");

    mainSearchBox->addItems( regexpTypes );
    quickFindSearchBox->addItems( regexpTypes );
}

// Convert a regexp type to its index in the list
int OptionsDialog::getRegexpIndex( SearchRegexpType syntax ) const
{
    return static_cast<int>( syntax );
}

// Convert the index of a regexp type to its type
SearchRegexpType OptionsDialog::getRegexpTypeFromIndex( int index ) const
{
    return static_cast<SearchRegexpType>( index );;
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

    // Regexp types
    mainSearchBox->setCurrentIndex(
            getRegexpIndex( Config().mainRegexpType() ) );
    quickFindSearchBox->setCurrentIndex(
            getRegexpIndex( Config().quickfindRegexpType() ) );
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

    Config().setMainRegexpType(
            getRegexpTypeFromIndex( mainSearchBox->currentIndex() ) );
    Config().setQuickfindRegexpType(
            getRegexpTypeFromIndex( quickFindSearchBox->currentIndex() ) );

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
