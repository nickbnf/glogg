/*
 * Copyright (C) 2009, 2010, 2012 Nicolas Bonnefon and other contributors
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

#ifndef FILTEREDVIEW_H
#define FILTEREDVIEW_H

#include "abstractlogview.h"

#include "data/logfiltereddata.h"

#include <QKeyEvent>

// Class implementing the filtered (bottom) view widget.
class FilteredView : public AbstractLogView
{
  Q_OBJECT
  public:
    FilteredView( LogFilteredData* newLogData,
            const QuickFindPattern* const quickFindPattern,
            QWidget* parent = 0 );

    // What is visible in the view.
    enum Visibility { MatchesOnly, MarksOnly, MarksAndMatches };
    void setVisibility( Visibility visi );

  protected:
    virtual LineType lineType(LineNumber lineNumber ) const;

    // Number of the filtered line relative to the unfiltered source
    virtual LineNumber displayLineNumber(LineNumber lineNumber ) const;
    virtual LineNumber lineIndex( LineNumber lineNumber ) const;
    virtual LineNumber maxDisplayLineNumber() const;

    virtual void keyPressEvent( QKeyEvent* keyEvent );

  private:
    LogFilteredData* logFilteredData_;
};

#endif
