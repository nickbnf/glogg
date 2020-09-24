/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2007 QMUL.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef KLOGG_ICONLOADER_H
#define KLOGG_ICONLOADER_H

#include <QIcon>

class QWidget;

class IconLoader {
  public:
    explicit IconLoader( QWidget* widget );
    QIcon load( QString name );

  private:
    bool shouldInvert() const;
    bool shouldAutoInvert( QString ) const;

    QPixmap loadPixmap( QString, int ) const;

    QPixmap invertPixmap( QPixmap ) const;

    QString makeNonScalableFilename( QString, int, bool ) const;

  private:
    QWidget* widget_;
};

#endif // KLOGG_ICONLOADER_H
