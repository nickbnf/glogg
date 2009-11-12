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

#include "log.h"

#include "configuration.h"
#include "filterset.h"

#include "filtersdialog.h"

// Construct the box, including a copy of the global FilterSet
// to handle ok/cancel/apply
FiltersDialog::FiltersDialog( QWidget* parent ) :
    QDialog( parent ), filterSet( Config().filterSet() )
{
    setupUi( this );


    populateColors();
    populateFilterList();

    connect( filterListWidget, SIGNAL( currentRowChanged( int ) ),
            this, SLOT( updatePropertyFields() ) );
    connect( patternEdit, SIGNAL( textEdited( const QString& ) ),
            this, SLOT( updateFilterProperties() ) );
    connect( foreColorBox, SIGNAL( activated( int ) ),
            this, SLOT( updateFilterProperties() ) );
    connect( backColorBox, SIGNAL( activated( int ) ),
            this, SLOT( updateFilterProperties() ) );
}

//
// Slots
//

void FiltersDialog::on_addFilterButton_clicked()
{
    LOG(logDEBUG) << "on_addFilterButton_clicked()";

    if ( !patternEdit->text().isEmpty() ) {
        Filter newFilter = Filter( patternEdit->text(),
                foreColorBox->currentText(), backColorBox->currentText() );
        filterSet.filterList << newFilter;

        // Add and select the newly created filter
        filterListWidget->addItem( patternEdit->text() );
        filterListWidget->setCurrentRow( filterListWidget->count() - 1 );
    }
}

void FiltersDialog::on_removeFilterButton_clicked()
{
    LOG(logDEBUG) << "on_removeFilterButton_clicked()";

    int index = filterListWidget->currentRow();
    if ( index >= 0 ) {
        delete filterListWidget->takeItem( index );
        filterSet.filterList.removeAt( index );
    }
}

void FiltersDialog::on_buttonBox_clicked( QAbstractButton* button )
{
    LOG(logDEBUG) << "on_buttonBox_clicked()";

    QDialogButtonBox::ButtonRole role = buttonBox->buttonRole( button );
    if (   ( role == QDialogButtonBox::AcceptRole )
        || ( role == QDialogButtonBox::ApplyRole ) ) {
        Config().filterSet() = filterSet;
        emit optionsChanged();
    }

    if ( role == QDialogButtonBox::AcceptRole )
        accept();
    else if ( role == QDialogButtonBox::RejectRole )
        reject();
}

void FiltersDialog::updatePropertyFields()
{
    LOG(logDEBUG) << "updatePropertyFields()";

    const Filter& currentFilter = filterSet.
        filterList.at( filterListWidget->currentRow() );

    patternEdit->setText( currentFilter.pattern() );
    int index = foreColorBox->findText( currentFilter.foreColorName() );
    if ( index != -1 ) {
        LOG(logDEBUG) << "fore index = " << index;
        foreColorBox->setCurrentIndex( index );
    }
    index = backColorBox->findText( currentFilter.backColorName() );
    if ( index != -1 ) {
        LOG(logDEBUG) << "back index = " << index;
        backColorBox->setCurrentIndex( index );
    }
}

void FiltersDialog::updateFilterProperties()
{
    LOG(logDEBUG) << "updateFilterProperties()";

    // If a row is selected
    if ( filterListWidget->currentRow() >= 0 ) {
        Filter& currentFilter = filterSet.
            filterList[filterListWidget->currentRow()];

        currentFilter.setPattern( patternEdit->text() );
        filterListWidget->currentItem()->setText( patternEdit->text() );
        currentFilter.setForeColor( foreColorBox->currentText() );
        currentFilter.setBackColor( backColorBox->currentText() );
    }
}

//
// Private functions
//

// Fills the color selection combo boxes
void FiltersDialog::populateColors()
{
    // const QStringList colorNames = QColor::colorNames();
    const QStringList colorNames = QStringList()
        // Basic 16 HTML colors (minus greys):
        << "black"
        << "white"
        << "maroon"
        << "red"
        << "purple"
        << "fuchsia"
        << "green"
        << "lime"
        << "olive"
        << "yellow"
        << "navy"
        << "blue"
        << "teal"
        << "aqua"
        // Greys
        << "gainsboro"
        << "lightgrey"
        << "silver"
        << "darkgrey"
        << "grey"
        << "dimgrey"
        // Reds
        << "tomato"
        << "orangered"
        << "orange"
        << "crimson"
        << "darkred"
        // Greens
        << "greenyellow"
        << "lightgreen"
        << "darkgreen"
        << "lightseagreen"
        // Blues
        << "lightcyan"
        << "darkturquoise"
        << "steelblue"
        << "lightblue"
        << "royalblue"
        << "darkblue"
        << "midnightblue"
        // Browns
        << "bisque"
        << "tan"
        << "sandybrown"
        << "chocolate";

    for ( QStringList::const_iterator i = colorNames.constBegin();
            i != colorNames.constEnd(); ++i ) {
        QPixmap solidPixmap( 20, 10 );
        solidPixmap.fill( QColor( *i ) );
        QIcon* solidIcon = new QIcon( solidPixmap );

        foreColorBox->addItem( *solidIcon, *i );
        backColorBox->addItem( *solidIcon, *i );
    }
}

void FiltersDialog::populateFilterList()
{
    filterListWidget->clear();
    foreach ( Filter filter, filterSet.filterList ) {
        filterListWidget->addItem( filter.pattern() );
    }
}
