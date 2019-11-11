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

#include "scratchpad.h"

#include <memory>

#include <QAction>
#include <QByteArray>
#include <QJsonDocument>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>

ScratchPad::ScratchPad( QWidget* parent )
    : QWidget( parent )
{
    this->hide();
    auto textEdit = std::make_unique<QTextEdit>();
    textEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    textEdit->setMinimumSize( 300, 300 );
    textEdit->setUndoRedoEnabled( true );

    auto toolBar = std::make_unique<QToolBar>();

    auto decodeBase64Action = std::make_unique<QAction>( "Decode base 64" );
    connect( decodeBase64Action.get(), &QAction::triggered, [this]( auto ) { decodeBase64(); } );
    toolBar->addAction( decodeBase64Action.release() );

    auto formatJsonAction = std::make_unique<QAction>( "Format json" );
    connect( formatJsonAction.get(), &QAction::triggered, [this]( auto ) { formatJson(); } );
    toolBar->addAction( formatJsonAction.release() );

    auto formatXmlAction = std::make_unique<QAction>( "Format xml" );
    connect( formatXmlAction.get(), &QAction::triggered, [this]( auto ) { formatXml(); } );
    // toolBar->addAction( formatXmlAction.release() );

    toolBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );

    auto layout = std::make_unique<QVBoxLayout>();
    layout->addWidget( toolBar.release() );
    layout->addWidget( textEdit.get() );

    textEdit_ = textEdit.release();
    this->setLayout( layout.release() );
}

void ScratchPad::decodeBase64()
{
    auto text = textEdit_->toPlainText();
    auto decoded = QByteArray::fromBase64( text.toLatin1() );
    auto decodedText = QString::fromStdString({ decoded.begin(), decoded.end() });
    if ( !decodedText.isEmpty() ) {
        textEdit_->setText( decodedText );
    }
}

void ScratchPad::formatJson()
{
    auto text = textEdit_->toPlainText();
    auto formatted = QJsonDocument::fromJson( text.toUtf8() ).toJson( QJsonDocument::Indented );
    if ( !formatted.isEmpty() ) {
        textEdit_->setText( formatted );
    }
}

void ScratchPad::formatXml() {}
