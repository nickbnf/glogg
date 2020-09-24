/*
 * Copyright (C) 2020 Anton Filimonov and other contributors
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

#ifndef KLOGG_FILEDIGEST_H
#define KLOGG_FILEDIGEST_H

#include <memory>
#include <QByteArray>

class DigestInternalState;

class FileDigest {
  public:
    FileDigest();
    ~FileDigest();

    FileDigest& addData( const char* data, size_t length );
    FileDigest& addData( const QByteArray& data );
    uint64_t digest() const;

    QByteArray hash() const;

    void reset();

  private:
    std::unique_ptr<DigestInternalState> m_state;
};

#endif // KLOGG_FILEDIGEST_H
