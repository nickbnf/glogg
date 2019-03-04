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

#ifndef MARKS_H
#define MARKS_H

#include <QChar>
#include <QList>

// Class encapsulating a single mark
// Contains the line number the mark is identifying.
class Mark {
  public:
    Mark( int line ) { lineNumber_ = line; };

    // Accessors
    int lineNumber() const { return lineNumber_; }

    bool operator <( const Mark& other ) const
    { return lineNumber_ < other.lineNumber_; }

  private:
    int lineNumber_;
};

// A list of marks, i.e. line numbers optionally associated to an
// identifying character.
class Marks {
  public:
    // Create an empty Marks
    Marks();

    // Add a mark at the given line, optionally identified by the given char
    // If a mark for this char already exist, the previous one is replaced.
    // It will happily add marks anywhere, even at stupid indexes.
    // Returns the index at which the mark was inserted,
    // such that getLineMarkedByIndex( addMark( line ) ) == line.
    int addMark( qint64 line, QChar mark = QChar() );
    // Get the (unique) mark identified by the passed char.
    qint64 getMark( QChar mark ) const;
    // Returns wheither the passed line has a mark on it.
    bool isLineMarked( qint64 line ) const;
    // Find a mark.
    // Returns the index of the mark or -1 if it was not found.
    int findMark( QChar mark );
    // Delete the mark at an index.
    void deleteMarkAt( int index );
    // Delete the mark present on the passed line or do nothing if there is
    // none.
    // Returns the index where the mark used to be.
    int deleteMark( qint64 line );
    // Get the line marked identified by the index (in this list) passed.
    qint64 getLineMarkedByIndex( int index ) const
    { return marks_[index].lineNumber(); }
    // Return the total number of marks
    unsigned size() const
    { return static_cast<unsigned>( marks_.size() ); }
    // Completely clear the marks list.
    void clear();

    // Iterator
    // Provide a const_iterator for the client to iterate through the marks.
    class const_iterator
            : public std::iterator<
                QList<Mark>::const_iterator::iterator_category,
                QList<Mark>::const_iterator::value_type,
                QList<Mark>::const_iterator::difference_type,
                QList<Mark>::const_iterator::pointer,
                QList<Mark>::const_iterator::reference>
    {
      public:
        const_iterator( QList<Mark>::const_iterator iter )
        { internal_iter_ = iter; }
        const_iterator( const const_iterator& original )
        { internal_iter_ = original.internal_iter_; }
        const Mark& operator*()
        { return *internal_iter_; }
        const Mark* operator->()
        { return &(*internal_iter_); }
        bool operator!=( const const_iterator& other ) const
        { return ( internal_iter_ != other.internal_iter_ ); }
        const_iterator& operator++()
        { ++internal_iter_ ; return *this; }
        const_iterator& operator--()
        { --internal_iter_ ; return *this; }

        int operator-( const const_iterator& other ) const
        { return ( internal_iter_ - other.internal_iter_ ); }

        const_iterator operator+(int n) const
        { return const_iterator(internal_iter_ + n); }
        const_iterator& operator+=(int n)
        { internal_iter_+=n ; return *this; }
        const_iterator operator-(int n) const
        { return const_iterator(internal_iter_ - n); }
        const_iterator& operator-=(int n)
        { internal_iter_-=n ; return *this; }

      private:
        QList<Mark>::const_iterator internal_iter_;
    };

    const_iterator begin() const
    { return const_iterator( marks_.begin() ); }
    const_iterator end() const
    { return const_iterator( marks_.end() ); }

  private:
    // List of marks.
    QList<Mark> marks_;
};

#endif
