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
#include "colorcombodelegate.h"

static const QString DEFAULT_PATTERN = "New Filter";
static const bool    DEFAULT_IGNORE_CASE = false;
static const QString DEFAULT_FORE_COLOUR = "black";
static const QString DEFAULT_BACK_COLOUR = "white";
static const QString DEFAULT_DESCRIPTION = "Description";

// Construct the box, including a copy of the global FilterSet
// to handle ok/cancel/apply
FiltersDialog::FiltersDialog( QWidget* parent ) : QDialog( parent )
{
    setupUi( this );

    // Reload the filter list from disk (in case it has been changed
    // by another glogg instance) and copy it to here.
    GetPersistentInfo().retrieve( "filterSet" );
    filterSet = PersistentCopy<FilterSet>( "filterSet" );

    // Start with all buttons disabled except 'add'
    removeFilterButton->setEnabled(false);
    upFilterButton->setEnabled(false);
    downFilterButton->setEnabled(false);

    filterTableView->setModel( filterSet.get() );
    filterTableView->horizontalHeader()->setStretchLastSection(true);
    filterTableView->verticalHeader()->setHidden(true);
    filterTableView->setColumnWidth(FilterSet::FILTER_COLUMN_DESCRIPTION, 180);
    filterTableView->setColumnWidth(FilterSet::FILTER_COLUMN_PATTERN, 180);
    filterTableView->setItemDelegateForColumn(FilterSet::FILTER_COLUMN_FOREGROUND, new ColorComboDelegate());
    filterTableView->setItemDelegateForColumn(FilterSet::FILTER_COLUMN_BACKGROUND, new ColorComboDelegate());
    filterTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Button status
    connect( filterSet.get(), SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),
             this, SLOT(updateButtonStatus()) );
    connect( filterTableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(updateButtonStatus()) );
}

//
// Slots
//

void FiltersDialog::on_addFilterButton_clicked()
{
    LOG(logDEBUG) << "on_addFilterButton_clicked()";

    Filter newFilter = Filter( DEFAULT_PATTERN, DEFAULT_IGNORE_CASE,
            DEFAULT_FORE_COLOUR, DEFAULT_BACK_COLOUR, DEFAULT_DESCRIPTION );

    // Add and select the newly created filter
    filterSet->addFilter( newFilter );
    filterTableView->selectRow( filterSet->rowCount(QModelIndex())-1 );
}

void FiltersDialog::on_removeFilterButton_clicked()
{
    QModelIndex modelIndex = filterTableView->currentIndex();
    if ( !modelIndex.isValid() )
    {
        return;
    }

    int index = modelIndex.row();
    LOG(logDEBUG) << "on_removeFilterButton_clicked() index " << index;

    filterSet->removeFilter(index);
    if ( index >= 0 )
    {
        int nextIndex = index;
        int numRows = filterSet->rowCount(QModelIndex());
        if ( nextIndex >= numRows-1 )
        {
            nextIndex--;
        }
        filterTableView->selectRow( nextIndex );
    }
}

void FiltersDialog::on_upFilterButton_clicked()
{
    QModelIndex modelIndex = filterTableView->currentIndex();
    if ( !modelIndex.isValid() )
    {
        return;
    }

    int index = modelIndex.row();
    LOG(logDEBUG) << "on_upFilterButton_clicked() index " << index;

    filterSet->moveFilterUp(index);
    filterTableView->selectRow( index > 0 ? index-1 : index );
}

void FiltersDialog::on_downFilterButton_clicked()
{
    QModelIndex modelIndex = filterTableView->currentIndex();
    if ( !modelIndex.isValid() )
    {
        return;
    }

    int index = modelIndex.row();
    LOG(logDEBUG) << "on_downFilterButton_clicked() index " << index;

    filterSet->moveFilterDown(index);

    int numRows = filterSet->rowCount(QModelIndex());
    if ( index >= 0 && index < numRows-1 )
    {
        filterTableView->selectRow( index+1 );
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

void FiltersDialog::updateButtonStatus()
{
    QModelIndex modelIndex = filterTableView->currentIndex();
    int index = -1;
    if ( modelIndex.isValid() )
    {
        index = modelIndex.row();
    }
    int numRows = filterSet->rowCount(QModelIndex());
    bool enabled = ( ( numRows > 0 ) &&
                     ( index >= 0 ) &&
                     ( index < numRows ) &&
                     ( !filterTableView->selectionModel()->selection().empty() ) );

    removeFilterButton->setEnabled( enabled );
    upFilterButton->setEnabled( enabled && index > 0 );
    downFilterButton->setEnabled( enabled && index < numRows-1 );
}
