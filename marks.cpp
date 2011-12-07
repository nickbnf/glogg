/*
 * Copyright (C) 2011 Nicolas Bonnefon and other contributors
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

#include "marks.h"

Marks::Marks()
{
}

void Marks::addMark( qint64 line, QChar mark )
{
}

qint64 Marks::getMark( QChar mark ) const
{
}

bool Marks::isLineMarked( qint64 line ) const
{
}

void Marks::deleteMark( QChar mark )
{
}

void Marks::deleteMark( qint64 line )
{
}

void Marks::clear()
{
}
