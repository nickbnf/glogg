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

static const char* DEFAULT_PATTERN = "New Highlighter";
static const bool    DEFAULT_IGNORE_CASE = false;

static const QColor DEFAULT_FORE_COLOUR("#000000");
static const QColor DEFAULT_BACK_COLOUR("#FFFFFF");

// Construct the box, including a copy of the global highlighterSet
// to handle ok/cancel/apply
HighlightersDialog::HighlightersDialog( QWidget* parent ) : QDialog( parent )
{
    setupUi( this );

    // Reload the highlighter list from disk (in case it has been changed
    // by another glogg instance) and copy it to here.
    highlighterSet_ = Persistable::getSynced<HighlighterSet>();

    populateHighlighterList();

    // Start with all buttons disabled except 'add'
    removeHighlighterButton->setEnabled(false);
    upHighlighterButton->setEnabled(false);
    downHighlighterButton->setEnabled(false);

    // Default to black on white
    updateIcon( foreColorButton , DEFAULT_FORE_COLOUR );
    updateIcon( backColorButton , DEFAULT_BACK_COLOUR );

    foreColor_ = DEFAULT_FORE_COLOUR;
    backColor_ = DEFAULT_BACK_COLOUR;

    // No highlighter selected by default
    selectedRow_ = -1;

    connect( highlighterListWidget, SIGNAL( itemSelectionChanged() ),
            this, SLOT( updatePropertyFields() ) );
    connect( patternEdit, SIGNAL( textEdited( const QString& ) ),
            this, SLOT( updateHighlighterProperties() ) );
    connect( ignoreCaseCheckBox, SIGNAL( clicked(bool) ),
            this, SLOT( updateHighlighterProperties() ) );

    if ( !highlighterSet_.highlighterList_.empty() ) {
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

    highlighterSet_.highlighterList_ << newHighlighter;

    // Add and select the newly created highlighter
    highlighterListWidget->addItem( DEFAULT_PATTERN );
    highlighterListWidget->setCurrentRow( highlighterListWidget->count() - 1 );
}

void HighlightersDialog::on_removeHighlighterButton_clicked()
{
    int index = highlighterListWidget->currentRow();
    LOG(logDEBUG) << "on_removeHighlighterButton_clicked() index " << index;

    if ( index >= 0 ) {
        highlighterSet_.highlighterList_.removeAt( index );
        highlighterListWidget->setCurrentRow( -1 );
        delete highlighterListWidget->takeItem( index );

        // Select the new item at the same index
        highlighterListWidget->setCurrentRow( index );

        int count = highlighterListWidget->count();
        if ( index < count ) {
            // Select the new item at the same index
            highlighterListWidget->setCurrentRow( index );
        }
        else {
            // or the previous index if it is at the end
            highlighterListWidget->setCurrentRow( count - 1 );
        }
    }
}

void HighlightersDialog::on_upHighlighterButton_clicked()
{
    int index = highlighterListWidget->currentRow();
    LOG(logDEBUG) << "on_upHighlighterButton_clicked() index " << index;

    if ( index > 0 ) {
        highlighterSet_.highlighterList_.move( index, index - 1 );

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
        highlighterSet_.highlighterList_.move( index, index + 1 );

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
        auto &persistentHighlighterSet = Persistable::getUnsynced<HighlighterSet>();
        persistentHighlighterSet = highlighterSet_;
        persistentHighlighterSet.save();
        emit optionsChanged();
    }

    if ( role == QDialogButtonBox::AcceptRole )
        accept();
    else if ( role == QDialogButtonBox::RejectRole )
        reject();
}

void HighlightersDialog::on_foreColorButton_clicked()
{
    // this method should never be called without a selected row
    // as all the property widgets should be disabled in this state
    if( selectedRow_ >= 0 ) {
        Highlighter& currentHighlighter = highlighterSet_.highlighterList_[ selectedRow_ ];

        QColor new_color;
        if ( showColorPicker( currentHighlighter.foreColor() , new_color ) ) {
            currentHighlighter.setForeColor(new_color);
            updateIcon(foreColorButton , currentHighlighter.foreColor());
            highlighterListWidget->currentItem()->setForeground( QBrush( new_color ) );
            foreColor_ = new_color;
        }
    }
}

void HighlightersDialog::on_backColorButton_clicked()
{
    // this method should never be called without a selected row
    // as all the property widgets should be disabled in this state
    if( selectedRow_ >= 0 ) {
        Highlighter& currentHighlighter = highlighterSet_.highlighterList_[ selectedRow_ ];

        QColor new_color;
        if ( showColorPicker( currentHighlighter.backColor() , new_color ) ) {
            currentHighlighter.setBackColor(new_color);
            updateIcon(backColorButton , currentHighlighter.backColor());
            highlighterListWidget->currentItem()->setBackground( QBrush( new_color ) );
            backColor_ = new_color;
        }
    }
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
        const Highlighter& currentHighlighter = highlighterSet_.highlighterList_.at( selectedRow_ );

        patternEdit->setText( currentHighlighter.pattern() );
        patternEdit->setEnabled( true );

        ignoreCaseCheckBox->setChecked( currentHighlighter.ignoreCase() );
        ignoreCaseCheckBox->setEnabled( true );

        updateIcon( foreColorButton , currentHighlighter.foreColor() );
        updateIcon( backColorButton , currentHighlighter.backColor() );

        foreColor_ = currentHighlighter.foreColor();
        backColor_ = currentHighlighter.backColor();

        // Enable the buttons if needed
        removeHighlighterButton->setEnabled( true );
        foreColorButton->setEnabled( true );
        backColorButton->setEnabled( true );
        upHighlighterButton->setEnabled( selectedRow_ > 0 );
        downHighlighterButton->setEnabled( selectedRow_ < ( highlighterListWidget->count() - 1 ) );
    }
    else {
        // Nothing is selected, reset and disable the controls
        patternEdit->clear();
        patternEdit->setEnabled( false );

        foreColorButton->setEnabled( false );
        backColorButton->setEnabled( false );
        updateIcon(foreColorButton , DEFAULT_FORE_COLOUR);
        updateIcon(backColorButton , DEFAULT_BACK_COLOUR);
        foreColor_ = DEFAULT_FORE_COLOUR;
        backColor_ = DEFAULT_BACK_COLOUR;

        ignoreCaseCheckBox->setChecked( DEFAULT_IGNORE_CASE );
        ignoreCaseCheckBox->setEnabled( false );
        removeHighlighterButton->setEnabled( false );
        upHighlighterButton->setEnabled( false );
        downHighlighterButton->setEnabled( false );
    }
}

void HighlightersDialog::updateHighlighterProperties()
{
    LOG(logDEBUG) << "updateHighlighterProperties()";

    // If a row is selected
    if ( selectedRow_ >= 0 ) {
        Highlighter& currentHighlighter = highlighterSet_.highlighterList_[selectedRow_];

        // Update the internal data
        currentHighlighter.setPattern( patternEdit->text() );
        currentHighlighter.setIgnoreCase( ignoreCaseCheckBox->isChecked() );
        currentHighlighter.setForeColor( foreColor_ );
        currentHighlighter.setBackColor( backColor_ );

        // Update the entry in the highlighterList widget
        highlighterListWidget->currentItem()->setText( patternEdit->text() );
        highlighterListWidget->currentItem()->setForeground(
                QBrush( QColor( currentHighlighter.foreColor() ) ) );
        highlighterListWidget->currentItem()->setBackground(
                QBrush( QColor( currentHighlighter.backColor() ) ) );
    }
}

//
// Private functions
//

void HighlightersDialog::updateIcon (QPushButton* button , QColor color)
{
    QPixmap CustomPixmap( 20, 10 );
    CustomPixmap.fill( color );
    button->setIcon(QIcon( CustomPixmap ));
}

bool HighlightersDialog::showColorPicker (const QColor& in , QColor& out)
{
    QColorDialog dialog;

    // non native dialog ensures they will have a default
    // set of colors to pick from in a pallette. For example,
    // on some linux desktops, the basic palette is missing
    dialog.setOption( QColorDialog::DontUseNativeDialog , true);

    dialog.setModal( true );
    dialog.setCurrentColor( in );
    dialog.exec();
    out = dialog.currentColor();

    return ( dialog.result() == QDialog::Accepted );
}

void HighlightersDialog::populateHighlighterList()
{
    highlighterListWidget->clear();
    for ( const Highlighter& highlighter : qAsConst(highlighterSet_.highlighterList_) ) {
        QListWidgetItem* new_item = new QListWidgetItem( highlighter.pattern() );
        // new_item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled );
        new_item->setForeground( QBrush( highlighter.foreColor() ) );
        new_item->setBackground( QBrush( highlighter.backColor() ) );
        highlighterListWidget->addItem( new_item );
    }
}
