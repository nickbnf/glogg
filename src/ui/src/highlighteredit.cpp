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

#include "highlighteredit.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QLineEdit>
#include <QPushButton>

HighlighterEdit::HighlighterEdit( Highlighter defaultHighlighter, QWidget* parent )
    : QWidget( parent )
    , defaultHighlighter_( std::move( defaultHighlighter ) )
{
    setupUi( this );

    reset();

    connect( patternEdit, &QLineEdit::textEdited, this, &HighlighterEdit::setPattern );
    connect( ignoreCaseCheckBox, &QCheckBox::toggled, this, &HighlighterEdit::setIgnoreCase );
    connect( onlyMatchCheckBox, &QCheckBox::toggled, this,
             &HighlighterEdit::setHighlightOnlyMatch );

    connect( foreColorButton, &QPushButton::clicked, this, &HighlighterEdit::changeForeColor );
    connect( backColorButton, &QPushButton::clicked, this, &HighlighterEdit::changeBackColor );
}

void HighlighterEdit::reset()
{
    patternEdit->clear();
    patternEdit->setEnabled( false );
    ignoreCaseCheckBox->setEnabled( false );
    foreColorButton->setEnabled( false );
    backColorButton->setEnabled( false );

    ignoreCaseCheckBox->setChecked( defaultHighlighter_.ignoreCase() );
    ignoreCaseCheckBox->setChecked( defaultHighlighter_.highlightOnlyMatch() );

    updateIcon( foreColorButton, defaultHighlighter_.foreColor() );
    updateIcon( backColorButton, defaultHighlighter_.backColor() );
}

void HighlighterEdit::setHighlighter( Highlighter highlighter )
{
    highlighter_ = std::move( highlighter );

    patternEdit->setText( highlighter_.pattern() );
    ignoreCaseCheckBox->setChecked( highlighter_.ignoreCase() );
    onlyMatchCheckBox->setChecked( highlighter_.highlightOnlyMatch() );
    updateIcon( foreColorButton, highlighter_.foreColor() );
    updateIcon( backColorButton, highlighter_.backColor() );

    patternEdit->setEnabled( true );
    ignoreCaseCheckBox->setEnabled( true );
    onlyMatchCheckBox->setEnabled( true );
    foreColorButton->setEnabled( true );
    backColorButton->setEnabled( true );
}

Highlighter HighlighterEdit::highlighter() const
{
    return highlighter_;
}

void HighlighterEdit::setPattern( const QString& pattern )
{
    highlighter_.setPattern( pattern );
    emit changed();
}

void HighlighterEdit::setIgnoreCase( bool ignoreCase )
{
    highlighter_.setIgnoreCase( ignoreCase );
    emit changed();
}

void HighlighterEdit::setHighlightOnlyMatch( bool onlyMatch )
{
    highlighter_.setHighlightOnlyMatch( onlyMatch );
    emit changed();
}

void HighlighterEdit::changeForeColor()
{
    QColor new_color;
    if ( showColorPicker( highlighter_.foreColor(), new_color ) ) {
        highlighter_.setForeColor( new_color );
        updateIcon( foreColorButton, highlighter_.foreColor() );
        emit changed();
    }
}

void HighlighterEdit::changeBackColor()
{
    QColor new_color;
    if ( showColorPicker( highlighter_.backColor(), new_color ) ) {
        highlighter_.setBackColor( new_color );
        updateIcon( backColorButton, highlighter_.backColor() );
        emit changed();
    }
}

void HighlighterEdit::updateIcon( QPushButton* button, const QColor& color )
{
    QPixmap CustomPixmap( 20, 10 );
    CustomPixmap.fill( color );
    button->setIcon( QIcon( CustomPixmap ) );
}

bool HighlighterEdit::showColorPicker( const QColor& in, QColor& out )
{
    QColorDialog dialog;

    // non native dialog ensures they will have a default
    // set of colors to pick from in a pallette. For example,
    // on some linux desktops, the basic palette is missing
    dialog.setOption( QColorDialog::DontUseNativeDialog, true );

    dialog.setModal( true );
    dialog.setCurrentColor( in );
    dialog.exec();
    out = dialog.currentColor();

    return ( dialog.result() == QDialog::Accepted );
}
