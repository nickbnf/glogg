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

#include "log.h"

#include "highlighterset.h"

#include "highlightersdialog.h"

#include <QColorDialog>
#include <QTimer>
#include <utility>

namespace {
static const char* DEFAULT_PATTERN = "New Highlighter";
static const bool DEFAULT_IGNORE_CASE = false;

static const QColor DEFAULT_FORE_COLOUR( "#000000" );
static const QColor DEFAULT_BACK_COLOUR( "#FFFFFF" );
} // namespace

// Construct the box, including a copy of the global highlighterSet
// to handle ok/cancel/apply
HighlightersDialog::HighlightersDialog( QWidget* parent )
    : QDialog( parent )
{
    setupUi( this );

    highlighterEdit_ = new HighlighterEdit(
        Highlighter{ "", DEFAULT_IGNORE_CASE, DEFAULT_FORE_COLOUR, DEFAULT_BACK_COLOUR }, this );
    highlighterEdit_->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    highlighterLayout->addWidget( highlighterEdit_ );

    // Reload the highlighter list from disk (in case it has been changed
    // by another glogg instance) and copy it to here.
    highlighterSet_ = HighlighterSet::getSynced();

    populateHighlighterList();

    // Start with all buttons disabled except 'add'
    removeHighlighterButton->setEnabled( false );
    upHighlighterButton->setEnabled( false );
    downHighlighterButton->setEnabled( false );

    connect( addHighlighterButton, &QToolButton::clicked, this,
             &HighlightersDialog::addHighlighter );
    connect( removeHighlighterButton, &QToolButton::clicked, this,
             &HighlightersDialog::removeHighlighter );

    connect( upHighlighterButton, &QToolButton::clicked, this,
             &HighlightersDialog::moveHighlighterUp );
    connect( downHighlighterButton, &QToolButton::clicked, this,
             &HighlightersDialog::moveHighlighterDown );

    // No highlighter selected by default
    selectedRow_ = -1;

    connect( highlighterListWidget, &QListWidget::itemSelectionChanged, this,
             &HighlightersDialog::updatePropertyFields );

    connect( highlighterEdit_, &HighlighterEdit::changed, this,
             &HighlightersDialog::updateHighlighterProperties );

    connect( buttonBox, &QDialogButtonBox::clicked, this, &HighlightersDialog::resolveDialog );

    if ( !highlighterSet_.highlighterList_.empty() ) {
        setCurrentRow( 0 );
    }
}

//
// Slots
//

void HighlightersDialog::addHighlighter()
{
    LOG( logDEBUG ) << "addHighlighter()";

    Highlighter newHighlighter = Highlighter( DEFAULT_PATTERN, DEFAULT_IGNORE_CASE,
                                              DEFAULT_FORE_COLOUR, DEFAULT_BACK_COLOUR );

    highlighterSet_.highlighterList_ << newHighlighter;

    // Add and select the newly created highlighter
    highlighterListWidget->addItem( DEFAULT_PATTERN );

    setCurrentRow( highlighterListWidget->count() - 1 );
}

void HighlightersDialog::removeHighlighter()
{
    int index = highlighterListWidget->currentRow();
    LOG( logDEBUG ) << "removeHighlighter() index " << index;

    if ( index >= 0 ) {
        highlighterSet_.highlighterList_.removeAt( index );
        highlighterListWidget->setCurrentRow( -1 );
        delete highlighterListWidget->takeItem( index );

        // Select the new item at the same index
        highlighterListWidget->setCurrentRow( index );

        int count = highlighterListWidget->count();
        if ( index < count ) {
            // Select the new item at the same index
            setCurrentRow( index );
        }
        else {
            // or the previous index if it is at the end
            setCurrentRow( count - 1 );
        }
    }
}

void HighlightersDialog::moveHighlighterUp()
{
    int index = highlighterListWidget->currentRow();
    LOG( logDEBUG ) << "moveHighlighterUp() index " << index;

    if ( index > 0 ) {
        highlighterSet_.highlighterList_.move( index, index - 1 );

        QListWidgetItem* item = highlighterListWidget->takeItem( index );
        highlighterListWidget->insertItem( index - 1, item );

        setCurrentRow( index - 1 );
    }
}

void HighlightersDialog::moveHighlighterDown()
{
    int index = highlighterListWidget->currentRow();
    LOG( logDEBUG ) << "moveHighlighterDown() index " << index;

    if ( ( index >= 0 ) && ( index < ( highlighterListWidget->count() - 1 ) ) ) {
        highlighterSet_.highlighterList_.move( index, index + 1 );

        QListWidgetItem* item = highlighterListWidget->takeItem( index );
        highlighterListWidget->insertItem( index + 1, item );

        setCurrentRow( index + 1 );
    }
}

void HighlightersDialog::resolveDialog( QAbstractButton* button )
{
    LOG( logDEBUG ) << "resolveDialog()";

    QDialogButtonBox::ButtonRole role = buttonBox->buttonRole( button );
    if ( role == QDialogButtonBox::RejectRole ) {
        reject();
        return;
    }

    // persist it to disk
    auto& persistentHighlighterSet = HighlighterSet::get();
    if ( role == QDialogButtonBox::AcceptRole ) {
        persistentHighlighterSet = std::move( highlighterSet_ );
        accept();
    }
    else if ( role == QDialogButtonBox::ApplyRole ) {
        persistentHighlighterSet = highlighterSet_;
    }
    else {
        LOG( logERROR ) << "unhandled role : " << role;
        return;
    }
    persistentHighlighterSet.save();
    emit optionsChanged();
}

void HighlightersDialog::setCurrentRow( int row )
{
    // ugly hack for mac
    QTimer::singleShot( 0, [this, row]() { highlighterListWidget->setCurrentRow( row ); } );
}

void HighlightersDialog::updatePropertyFields()
{
    if ( highlighterListWidget->selectedItems().count() >= 1 )
        selectedRow_ = highlighterListWidget->row( highlighterListWidget->selectedItems().at( 0 ) );
    else
        selectedRow_ = -1;

    LOG( logDEBUG ) << "updatePropertyFields(), row = " << selectedRow_;

    if ( selectedRow_ >= 0 ) {
        const Highlighter& currentHighlighter = highlighterSet_.highlighterList_.at( selectedRow_ );
        highlighterEdit_->setHighlighter( currentHighlighter );

        // Enable the buttons if needed
        removeHighlighterButton->setEnabled( true );
        upHighlighterButton->setEnabled( selectedRow_ > 0 );
        downHighlighterButton->setEnabled( selectedRow_ < ( highlighterListWidget->count() - 1 ) );
    }
    else {
        highlighterEdit_->reset();

        removeHighlighterButton->setEnabled( false );
        upHighlighterButton->setEnabled( false );
        downHighlighterButton->setEnabled( false );
    }
}

void HighlightersDialog::updateHighlighterProperties()
{
    LOG( logDEBUG ) << "updateHighlighterProperties()";

    // If a row is selected
    if ( selectedRow_ >= 0 ) {
        Highlighter& currentHighlighter = highlighterSet_.highlighterList_[ selectedRow_ ];

        currentHighlighter = highlighterEdit_->highlighter();

        // Update the entry in the highlighterList widget
        highlighterListWidget->currentItem()->setText( currentHighlighter.pattern() );
        highlighterListWidget->currentItem()->setForeground(
            QBrush( currentHighlighter.foreColor() ) );
        highlighterListWidget->currentItem()->setBackground(
            QBrush( currentHighlighter.backColor() ) );
    }
}

//
// Private functions
//

void HighlightersDialog::populateHighlighterList()
{
    highlighterListWidget->clear();
    for ( const Highlighter& highlighter : qAsConst( highlighterSet_.highlighterList_ ) ) {
        auto* new_item = new QListWidgetItem( highlighter.pattern() );
        // new_item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled );
        new_item->setForeground( QBrush( highlighter.foreColor() ) );
        new_item->setBackground( QBrush( highlighter.backColor() ) );
        highlighterListWidget->addItem( new_item );
    }
}
