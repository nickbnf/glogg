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

#include <QFileDialog>
#include <QTimer>

#include <utility>

#include "highlightersdialog.h"
#include "highlighterset.h"
#include "iconloader.h"
#include "log.h"

static constexpr QLatin1String DEFAULT_NAME = QLatin1String( "New Highlighter set", 19 );

// Construct the box, including a copy of the global highlighterSet
// to handle ok/cancel/apply
HighlightersDialog::HighlightersDialog( QWidget* parent )
    : QDialog( parent )
{
    setupUi( this );

    highlighterSetEdit_ = new HighlighterSetEdit( this );
    highlighterSetEdit_->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    highlighterLayout->addWidget( highlighterSetEdit_ );

    splitter->setStretchFactor( 0, 0 );
    splitter->setStretchFactor( 1, 1 );

    // Reload the highlighter list from disk (in case it has been changed
    // by another glogg instance) and copy it to here.
    highlighterSetCollection_ = HighlighterSetCollection::getSynced();

    populateHighlighterList();

    // Start with all buttons disabled except 'add'
    removeHighlighterButton->setEnabled( false );
    upHighlighterButton->setEnabled( false );
    downHighlighterButton->setEnabled( false );

    connect( addHighlighterButton, &QToolButton::clicked, this,
             &HighlightersDialog::addHighlighterSet );
    connect( removeHighlighterButton, &QToolButton::clicked, this,
             &HighlightersDialog::removeHighlighterSet );

    connect( upHighlighterButton, &QToolButton::clicked, this,
             &HighlightersDialog::moveHighlighterSetUp );
    connect( downHighlighterButton, &QToolButton::clicked, this,
             &HighlightersDialog::moveHighlighterSetDown );

    connect( exportButton, &QPushButton::clicked, this, &HighlightersDialog::exportHighlighters );
    connect( importButton, &QPushButton::clicked, this, &HighlightersDialog::importHighlighters );

    // No highlighter selected by default
    selectedRow_ = -1;

    connect( highlighterListWidget, &QListWidget::itemSelectionChanged, this,
             &HighlightersDialog::updatePropertyFields );

    connect( highlighterSetEdit_, &HighlighterSetEdit::changed, this,
             &HighlightersDialog::updateHighlighterProperties );

    connect( buttonBox, &QDialogButtonBox::clicked, this, &HighlightersDialog::resolveDialog );

    if ( !highlighterSetCollection_.highlighters_.empty() ) {
        setCurrentRow( 0 );
    }

    QTimer::singleShot( 0, [this] {
        IconLoader iconLoader( this );

        addHighlighterButton->setIcon( iconLoader.load( "icons8-plus-16" ) );
        removeHighlighterButton->setIcon( iconLoader.load( "icons8-minus-16" ) );
        upHighlighterButton->setIcon( iconLoader.load( "icons8-up-16" ) );
        downHighlighterButton->setIcon( iconLoader.load( "icons8-down-arrow-16" ) );
    } );
}

//
// Slots
//

void HighlightersDialog::exportHighlighters()
{
    QString file = QFileDialog::getSaveFileName( this, "Export highlighters configuration", "",
                                                 "Highlighters (*.conf)" );

    if ( file.isEmpty() ) {
        return;
    }

    QSettings settings{ file, QSettings::IniFormat };
    highlighterSetCollection_.saveToStorage( settings );
}

void HighlightersDialog::importHighlighters()
{
    QStringList files = QFileDialog::getOpenFileNames( this, "Select one or more files to open", "",
                                                       "Highlighters (*.conf)" );

    for ( const auto& file : qAsConst( files ) ) {
        LOG( logDEBUG ) << "Loading highlighters from " << file;
        QSettings settings{ file, QSettings::IniFormat };
        HighlighterSetCollection collection;
        collection.retrieveFromStorage( settings );
        for ( const auto& set : qAsConst( collection.highlighters_ ) ) {
            if ( highlighterSetCollection_.hasSet( set.id() ) ) {
                continue;
            }

            highlighterSetCollection_.highlighters_.append( set );
            highlighterListWidget->addItem( set.name() );
        }
    }
}

void HighlightersDialog::addHighlighterSet()
{
    LOG( logDEBUG ) << "addHighlighter()";

    highlighterSetCollection_.highlighters_.append( HighlighterSet::createNewSet( DEFAULT_NAME ) );

    // Add and select the newly created highlighter
    highlighterListWidget->addItem( DEFAULT_NAME );

    setCurrentRow( highlighterListWidget->count() - 1 );
}

void HighlightersDialog::removeHighlighterSet()
{
    int index = highlighterListWidget->currentRow();
    LOG( logDEBUG ) << "removeHighlighter() index " << index;

    if ( index >= 0 ) {
        setCurrentRow( -1 );
        QTimer::singleShot( 0, [this, index] {
            {
                const auto& set = highlighterSetCollection_.highlighters_.at( index );
                if ( set.id() == highlighterSetCollection_.currentSetId() ) {
                    highlighterSetCollection_.setCurrentSet( {} );
                }
            }

            highlighterSetCollection_.highlighters_.removeAt( index );
            delete highlighterListWidget->takeItem( index );

            int count = highlighterListWidget->count();
            if ( index < count ) {
                // Select the new item at the same index
                setCurrentRow( index );
            }
            else {
                // or the previous index if it is at the end
                setCurrentRow( count - 1 );
            }
        } );
    }
}

void HighlightersDialog::moveHighlighterSetUp()
{
    int index = highlighterListWidget->currentRow();
    LOG( logDEBUG ) << "moveHighlighterUp() index " << index;

    if ( index > 0 ) {
        highlighterSetCollection_.highlighters_.move( index, index - 1 );

        QTimer::singleShot( 0, [this, index] {
            QListWidgetItem* item = highlighterListWidget->takeItem( index );
            highlighterListWidget->insertItem( index - 1, item );

            setCurrentRow( index - 1 );
        } );
    }
}

void HighlightersDialog::moveHighlighterSetDown()
{
    int index = highlighterListWidget->currentRow();
    LOG( logDEBUG ) << "moveHighlighterDown() index " << index;

    if ( ( index >= 0 ) && ( index < ( highlighterListWidget->count() - 1 ) ) ) {
        highlighterSetCollection_.highlighters_.move( index, index + 1 );

        QTimer::singleShot( 0, [this, index] {
            QListWidgetItem* item = highlighterListWidget->takeItem( index );
            highlighterListWidget->insertItem( index + 1, item );

            setCurrentRow( index + 1 );
        } );
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
    auto& persistentHighlighterSet = HighlighterSetCollection::get();
    if ( role == QDialogButtonBox::AcceptRole ) {
        persistentHighlighterSet = std::move( highlighterSetCollection_ );
        accept();
    }
    else if ( role == QDialogButtonBox::ApplyRole ) {
        persistentHighlighterSet = highlighterSetCollection_;
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

void HighlightersDialog::updateGroupTitle( const HighlighterSet& set )
{
    groupBox->setTitle( QString( "Set %1 properties" ).arg( set.name() ) );
}

void HighlightersDialog::updatePropertyFields()
{
    if ( highlighterListWidget->selectedItems().count() >= 1 )
        selectedRow_ = highlighterListWidget->row( highlighterListWidget->selectedItems().at( 0 ) );
    else
        selectedRow_ = -1;

    LOG( logDEBUG ) << "updatePropertyFields(), row = " << selectedRow_;

    if ( selectedRow_ >= 0 ) {
        const HighlighterSet& currentSet
            = highlighterSetCollection_.highlighters_.at( selectedRow_ );
        highlighterSetEdit_->setHighlighters( currentSet );

        // Enable the buttons if needed
        removeHighlighterButton->setEnabled( true );
        upHighlighterButton->setEnabled( selectedRow_ > 0 );
        downHighlighterButton->setEnabled( selectedRow_ < ( highlighterListWidget->count() - 1 ) );

        updateGroupTitle( currentSet );
    }
    else {
        highlighterSetEdit_->reset();

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
        HighlighterSet& currentSet = highlighterSetCollection_.highlighters_[ selectedRow_ ];
        currentSet = highlighterSetEdit_->highlighters();
        // Update the entry in the highlighterList widget
        highlighterListWidget->currentItem()->setText( currentSet.name() );
        updateGroupTitle( currentSet );
    }
}

//
// Private functions
//

void HighlightersDialog::populateHighlighterList()
{
    highlighterListWidget->clear();
    for ( const HighlighterSet& highlighterSet :
          qAsConst( highlighterSetCollection_.highlighters_ ) ) {
        auto* new_item = new QListWidgetItem( highlighterSet.name() );
        // new_item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled );
        highlighterListWidget->addItem( new_item );
    }
}
