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

#include "log.h"

#include "configuration.h"
#include "persistentinfo.h"
#include "filterset.h"

#include "filtersdialog.h"

static const QString DEFAULT_PATTERN = "New Filter";
static const bool    DEFAULT_IGNORE_CASE = false;
static const QString DEFAULT_FORE_COLOUR = "black";
static const QString DEFAULT_BACK_COLOUR = "white";

// Construct the box, including a copy of the global FilterSet
// to handle ok/cancel/apply
FiltersDialog::FiltersDialog( QWidget* parent ) : QDialog( parent )
{
    setupUi( this );

    // Reload the filter list from disk (in case it has been changed
    // by another glogg instance) and copy it to here.
    GetPersistentInfo().retrieve( "filterSet" );
    filterSet = PersistentCopy<FilterSet>( "filterSet" );

    populateColors();
    populateFilterList();

    // Start with all buttons disabled except 'add'
    removeFilterButton->setEnabled(false);
    upFilterButton->setEnabled(false);
    downFilterButton->setEnabled(false);

    // Default to black on white
    int index = foreColorBox->findText( DEFAULT_FORE_COLOUR );
    foreColorBox->setCurrentIndex( index );
    index = backColorBox->findText( DEFAULT_BACK_COLOUR );
    backColorBox->setCurrentIndex( index );

    // No filter selected by default
    selectedRow_ = -1;

    connect( filterListWidget, SIGNAL( itemSelectionChanged() ),
            this, SLOT( updatePropertyFields() ) );
    connect( patternEdit, SIGNAL( textEdited( const QString& ) ),
            this, SLOT( updateFilterProperties() ) );
    connect( ignoreCaseCheckBox, SIGNAL( toggled(bool) ),
            this, SLOT( updateFilterProperties() ) );
    connect( foreColorBox, SIGNAL( activated( int ) ),
            this, SLOT( updateFilterProperties() ) );
    connect( backColorBox, SIGNAL( activated( int ) ),
            this, SLOT( updateFilterProperties() ) );

    if ( !filterSet->filterList.empty() ) {
        filterListWidget->setCurrentItem( filterListWidget->item( 0 ) );
    }
}

//
// Slots
//

void FiltersDialog::on_addFilterButton_clicked()
{
    LOG(logDEBUG) << "on_addFilterButton_clicked()";

    Filter newFilter = Filter( DEFAULT_PATTERN, DEFAULT_IGNORE_CASE,
            DEFAULT_FORE_COLOUR, DEFAULT_BACK_COLOUR );
    filterSet->filterList << newFilter;

    // Add and select the newly created filter
    filterListWidget->addItem( DEFAULT_PATTERN );
    filterListWidget->setCurrentRow( filterListWidget->count() - 1 );
}

void FiltersDialog::on_removeFilterButton_clicked()
{
    int index = filterListWidget->currentRow();
    LOG(logDEBUG) << "on_removeFilterButton_clicked() index " << index;

    if ( index >= 0 ) {
        filterSet->filterList.removeAt( index );
        filterListWidget->setCurrentRow( -1 );
        delete filterListWidget->takeItem( index );

        // Select the new item at the same index
        filterListWidget->setCurrentRow( index );
    }
}

void FiltersDialog::on_upFilterButton_clicked()
{
    int index = filterListWidget->currentRow();
    LOG(logDEBUG) << "on_upFilterButton_clicked() index " << index;

    if ( index > 0 ) {
        filterSet->filterList.move( index, index - 1 );

        QListWidgetItem* item = filterListWidget->takeItem( index );
        filterListWidget->insertItem( index - 1, item );
        filterListWidget->setCurrentRow( index - 1 );
    }
}

void FiltersDialog::on_downFilterButton_clicked()
{
    int index = filterListWidget->currentRow();
    LOG(logDEBUG) << "on_downFilterButton_clicked() index " << index;

    if ( ( index >= 0 ) && ( index < ( filterListWidget->count() - 1 ) ) ) {
        filterSet->filterList.move( index, index + 1 );

        QListWidgetItem* item = filterListWidget->takeItem( index );
        filterListWidget->insertItem( index + 1, item );
        filterListWidget->setCurrentRow( index + 1 );
    }
}

void FiltersDialog::on_buttonBox_clicked( QAbstractButton* button )
{
    LOG(logDEBUG) << "on_buttonBox_clicked()";

    QDialogButtonBox::ButtonRole role = buttonBox->buttonRole( button );
    if (   ( role == QDialogButtonBox::AcceptRole )
        || ( role == QDialogButtonBox::ApplyRole ) ) {
        // Copy the filter set and persist it to disk
        *( Persistent<FilterSet>( "filterSet" ) ) = *filterSet;
        GetPersistentInfo().save( "filterSet" );
        emit optionsChanged();
    }

    if ( role == QDialogButtonBox::AcceptRole )
        accept();
    else if ( role == QDialogButtonBox::RejectRole )
        reject();
}

void FiltersDialog::updatePropertyFields()
{
    if ( filterListWidget->selectedItems().count() >= 1 )
        selectedRow_ = filterListWidget->row(
                filterListWidget->selectedItems().at(0) );
    else
        selectedRow_ = -1;

    LOG(logDEBUG) << "updatePropertyFields(), row = " << selectedRow_;

    if ( selectedRow_ >= 0 ) {
        const Filter& currentFilter = filterSet->filterList.at( selectedRow_ );

        patternEdit->setText( currentFilter.pattern() );
        patternEdit->setEnabled( true );

        ignoreCaseCheckBox->setChecked( currentFilter.ignoreCase() );
        ignoreCaseCheckBox->setEnabled( true );

        int index = foreColorBox->findText( currentFilter.foreColorName() );
        if ( index != -1 ) {
            LOG(logDEBUG) << "fore index = " << index;
            foreColorBox->setCurrentIndex( index );
            foreColorBox->setEnabled( true );
        }
        index = backColorBox->findText( currentFilter.backColorName() );
        if ( index != -1 ) {
            LOG(logDEBUG) << "back index = " << index;
            backColorBox->setCurrentIndex( index );
            backColorBox->setEnabled( true );
        }

        // Enable the buttons if needed
        removeFilterButton->setEnabled( true );
        upFilterButton->setEnabled( ( selectedRow_ > 0 ) ? true : false );
        downFilterButton->setEnabled(
                ( selectedRow_ < ( filterListWidget->count() - 1 ) ) ? true : false );
    }
    else {
        // Nothing is selected, greys the buttons
        patternEdit->setEnabled( false );
        foreColorBox->setEnabled( false );
        backColorBox->setEnabled( false );
        ignoreCaseCheckBox->setEnabled( false );
    }
}

void FiltersDialog::updateFilterProperties()
{
    LOG(logDEBUG) << "updateFilterProperties()";

    // If a row is selected
    if ( selectedRow_ >= 0 ) {
        Filter& currentFilter = filterSet->filterList[selectedRow_];

        // Update the internal data
        currentFilter.setPattern( patternEdit->text() );
        currentFilter.setIgnoreCase( ignoreCaseCheckBox->isChecked() );
        currentFilter.setForeColor( foreColorBox->currentText() );
        currentFilter.setBackColor( backColorBox->currentText() );

        // Update the entry in the filterList widget
        filterListWidget->currentItem()->setText( patternEdit->text() );
        filterListWidget->currentItem()->setForeground(
                QBrush( QColor( currentFilter.foreColorName() ) ) );
        filterListWidget->currentItem()->setBackground(
                QBrush( QColor( currentFilter.backColorName() ) ) );
    }
}

//
// Private functions
//

// Fills the color selection combo boxes
void FiltersDialog::populateColors()
{
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
    foreach ( Filter filter, filterSet->filterList ) {
        QListWidgetItem* new_item = new QListWidgetItem( filter.pattern() );
        // new_item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled );
        new_item->setForeground( QBrush( QColor( filter.foreColorName() ) ) );
        new_item->setBackground( QBrush( QColor( filter.backColorName() ) ) );
        filterListWidget->addItem( new_item );
    }
}
