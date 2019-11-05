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

#ifndef SCRATCHPAD_H
#define SCRATCHPAD_H

#include <QWidget>

class QTextEdit;

class ScratchPad : public QWidget {
    Q_OBJECT
  public:
    ScratchPad( QWidget* parent = nullptr );

    ~ScratchPad() = default;
    ScratchPad( const ScratchPad& ) = delete;
    ScratchPad& operator=( const ScratchPad& ) = delete;

  private:
    void decodeBase64();
    void formatJson();
    void formatXml();

  private:
    QTextEdit* textEdit_;
};

#endif // SCRATCHPAD_H