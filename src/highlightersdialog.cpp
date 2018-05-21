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
#include "highlighterset.h"

#include "highlightersdialog.h"

static const QString DEFAULT_PATTERN = "New Highlighter";
static const bool    DEFAULT_IGNORE_CASE = false;
static const QString DEFAULT_FORE_COLOUR = "black";
static const QString DEFAULT_BACK_COLOUR = "white";

// Construct the box, including a copy of the global HighlighterSet
// to handle ok/cancel/apply
HighlightersDialog::HighlightersDialog( QWidget* parent ) : QDialog( parent )
{
    setupUi( this );

    // Reload the highlighter list from disk (in case it has been changed
    // by another glogg instance) and copy it to here.
    GetPersistentInfo().retrieve( "highlighterSet" );
    highlighterSet = PersistentCopy<HighlighterSet>( "highlighterSet" );

    populateColors();
    populateHighlighterList();

    // Start with all buttons disabled except 'add'
    removeHighlighterButton->setEnabled(false);
    upHighlighterButton->setEnabled(false);
    downHighlighterButton->setEnabled(false);

    // Default to black on white
    int index = foreColorBox->findText( DEFAULT_FORE_COLOUR );
    foreColorBox->setCurrentIndex( index );
    index = backColorBox->findText( DEFAULT_BACK_COLOUR );
    backColorBox->setCurrentIndex( index );

    // No highlighter selected by default
    selectedRow_ = -1;

    connect( highlighterListWidget, SIGNAL( itemSelectionChanged() ),
            this, SLOT( updatePropertyFields() ) );
    connect( patternEdit, SIGNAL( textEdited( const QString& ) ),
            this, SLOT( updateHighlighterProperties() ) );
    connect( ignoreCaseCheckBox, SIGNAL( clicked( bool ) ),
            this, SLOT( updateHighlighterProperties() ) );
    connect( foreColorBox, SIGNAL( activated( int ) ),
            this, SLOT( updateHighlighterProperties() ) );
    connect( backColorBox, SIGNAL( activated( int ) ),
            this, SLOT( updateHighlighterProperties() ) );

    if ( !highlighterSet->highlighterList.empty() ) {
        highlighterListWidget->setCurrentItem( highlighterListWidget->item( 0 ) );
    }
}

//
// Slots
//

void HighlightersDialog::on_addHighlighterButton_clicked()
{
    LOG(logDEBUG) << "on_addHighlighterButton_clicked()";

    Highlighter newHighlighter = Highlighter( DEFAULT_PATTERN, DEFAULT_IGNORE_CASE,
            DEFAULT_FORE_COLOUR, DEFAULT_BACK_COLOUR );
    highlighterSet->highlighterList << newHighlighter;

    // Add and select the newly created highlighter
    highlighterListWidget->addItem( DEFAULT_PATTERN );
    highlighterListWidget->setCurrentRow( highlighterListWidget->count() - 1 );
}

void HighlightersDialog::on_removeHighlighterButton_clicked()
{
    int index = highlighterListWidget->currentRow();
    LOG(logDEBUG) << "on_removeHighlighterButton_clicked() index " << index;

    if ( index >= 0 ) {
        highlighterSet->highlighterList.removeAt( index );
        highlighterListWidget->setCurrentRow( -1 );
        delete highlighterListWidget->takeItem( index );

        // Select the new item at the same index
        highlighterListWidget->setCurrentRow( index );
    }
}

void HighlightersDialog::on_upHighlighterButton_clicked()
{
    int index = highlighterListWidget->currentRow();
    LOG(logDEBUG) << "on_upHighlighterButton_clicked() index " << index;

    if ( index > 0 ) {
        highlighterSet->highlighterList.move( index, index - 1 );

        QListWidgetItem* item = highlighterListWidget->takeItem( index );
        highlighterListWidget->insertItem( index - 1, item );
        highlighterListWidget->setCurrentRow( index - 1 );
    }
}

void HighlightersDialog::on_downHighlighterButton_clicked()
{
    int index = highlighterListWidget->currentRow();
    LOG(logDEBUG) << "on_downHighlighterButton_clicked() index " << index;

    if ( ( index >= 0 ) && ( index < ( highlighterListWidget->count() - 1 ) ) ) {
        highlighterSet->highlighterList.move( index, index + 1 );

        QListWidgetItem* item = highlighterListWidget->takeItem( index );
        highlighterListWidget->insertItem( index + 1, item );
        highlighterListWidget->setCurrentRow( index + 1 );
    }
}

void HighlightersDialog::on_buttonBox_clicked( QAbstractButton* button )
{
    LOG(logDEBUG) << "on_buttonBox_clicked()";

    QDialogButtonBox::ButtonRole role = buttonBox->buttonRole( button );
    if (   ( role == QDialogButtonBox::AcceptRole )
        || ( role == QDialogButtonBox::ApplyRole ) ) {
        // Copy the highlighter set and persist it to disk
        *( Persistent<HighlighterSet>( "highlighterSet" ) ) = *highlighterSet;
        GetPersistentInfo().save( "highlighterSet" );
        emit optionsChanged();
    }

    if ( role == QDialogButtonBox::AcceptRole )
        accept();
    else if ( role == QDialogButtonBox::RejectRole )
        reject();
}

void HighlightersDialog::updatePropertyFields()
{
    if ( highlighterListWidget->selectedItems().count() >= 1 )
        selectedRow_ = highlighterListWidget->row(
                highlighterListWidget->selectedItems().at(0) );
    else
        selectedRow_ = -1;

    LOG(logDEBUG) << "updatePropertyFields(), row = " << selectedRow_;

    if ( selectedRow_ >= 0 ) {
        const Highlighter& currentHighlighter = highlighterSet->highlighterList.at( selectedRow_ );

        patternEdit->setText( currentHighlighter.pattern() );
        patternEdit->setEnabled( true );

        ignoreCaseCheckBox->setChecked( currentHighlighter.ignoreCase() );
        ignoreCaseCheckBox->setEnabled( true );

        int index = foreColorBox->findText( currentHighlighter.foreColorName() );
        if ( index != -1 ) {
            LOG(logDEBUG) << "fore index = " << index;
            foreColorBox->setCurrentIndex( index );
            foreColorBox->setEnabled( true );
        }
        index = backColorBox->findText( currentHighlighter.backColorName() );
        if ( index != -1 ) {
            LOG(logDEBUG) << "back index = " << index;
            backColorBox->setCurrentIndex( index );
            backColorBox->setEnabled( true );
        }

        // Enable the buttons if needed
        removeHighlighterButton->setEnabled( true );
        upHighlighterButton->setEnabled( ( selectedRow_ > 0 ) ? true : false );
        downHighlighterButton->setEnabled(
                ( selectedRow_ < ( highlighterListWidget->count() - 1 ) ) ? true : false );
    }
    else {
        // Nothing is selected, greys the buttons
        patternEdit->setEnabled( false );
        foreColorBox->setEnabled( false );
        backColorBox->setEnabled( false );
        ignoreCaseCheckBox->setEnabled( false );
    }
}

void HighlightersDialog::updateHighlighterProperties()
{
    LOG(logDEBUG) << "updateHighlighterProperties()";

    // If a row is selected
    if ( selectedRow_ >= 0 ) {
        Highlighter& currentHighlighter = highlighterSet->highlighterList[selectedRow_];

        // Update the internal data
        currentHighlighter.setPattern( patternEdit->text() );
        currentHighlighter.setIgnoreCase( ignoreCaseCheckBox->isChecked() );
        currentHighlighter.setForeColor( foreColorBox->currentText() );
        currentHighlighter.setBackColor( backColorBox->currentText() );

        // Update the entry in the highlighterList widget
        highlighterListWidget->currentItem()->setText( patternEdit->text() );
        highlighterListWidget->currentItem()->setForeground(
                QBrush( QColor( currentHighlighter.foreColorName() ) ) );
        highlighterListWidget->currentItem()->setBackground(
                QBrush( QColor( currentHighlighter.backColorName() ) ) );
    }
}

//
// Private functions
//

// Fills the color selection combo boxes
void HighlightersDialog::populateColors()
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
        QIcon solidIcon { solidPixmap };

        foreColorBox->addItem( solidIcon, *i );
        backColorBox->addItem( solidIcon, *i );
    }
}

void HighlightersDialog::populateHighlighterList()
{
    highlighterListWidget->clear();
    foreach ( Highlighter highlighter, highlighterSet->highlighterList ) {
        QListWidgetItem* new_item = new QListWidgetItem( highlighter.pattern() );
        // new_item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled );
        new_item->setForeground( QBrush( QColor( highlighter.foreColorName() ) ) );
        new_item->setBackground( QBrush( QColor( highlighter.backColorName() ) ) );
        highlighterListWidget->addItem( new_item );
    }
}
