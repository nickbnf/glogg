#include "drawhelpers.h"
#include <QPainter>
#include <QFontMetrics>
#include <QString>
#include "log.h"


LineChunk::LineChunk( int first_col, int last_col, ChunkType type )
{
    // LOG(logDEBUG) << "new LineChunk: " << first_col << " " << last_col;

    start_ = first_col;
    end_   = last_col;
    type_  = type;
}

QList<LineChunk> LineChunk::select( int sel_start, int sel_end ) const
{
    QList<LineChunk> list;

    if ( ( sel_start < start_ ) && ( sel_end < start_ ) ) {
        // Selection BEFORE this chunk: no change
        list << LineChunk( *this );
    }
    else if ( sel_start > end_ ) {
        // Selection AFTER this chunk: no change
        list << LineChunk( *this );
    }
    else /* if ( ( sel_start >= start_ ) && ( sel_end <= end_ ) ) */
    {
        // We only want to consider what's inside THIS chunk
        sel_start = qMax( sel_start, start_ );
        sel_end   = qMin( sel_end, end_ );

        if ( sel_start > start_ ) {
            list << LineChunk( start_, sel_start - 1, type_ );
        }

        list << LineChunk( sel_start, sel_end, Selected );

        if ( sel_end < end_ ){
            list << LineChunk( sel_end + 1, end_, type_ );
        }
    }

    return list;
}

QList<LineChunk> LineChunk::selectFiltered(LineChunk chunk) const
{
    int sel_start = chunk.start();
    int sel_end = chunk.end() - 1;

    QList<LineChunk> list;

    if(Normal != type_){
        list << LineChunk( *this );
        return list;
    }

    if ( ( sel_start < start_ ) && ( sel_end < start_ ) ) {
        // Selection BEFORE this chunk: no change
        list << LineChunk( *this );
    }
    else if ( sel_start > end_ ) {
        // Selection AFTER this chunk: no change
        list << LineChunk( *this );
    }
    else /* if ( ( sel_start >= start_ ) && ( sel_end <= end_ ) ) */
    {
        // We only want to consider what's inside THIS chunk
        sel_start = qMax( sel_start, start_ );
        sel_end   = qMin( sel_end, end_ );

        if ( sel_start > start_ ) {
            list << LineChunk( start_, sel_start - 1, type_ );
        }

        list << LineChunk( sel_start, sel_end, chunk.foreground(), chunk.background() );

        if ( sel_end < end_ ){
            list << LineChunk( sel_end + 1, end_, type_ );
        }
    }

    return list;
}

void LineDrawer::addChunk( int first_col, int last_col,
        QColor fore, QColor back )
{
    if ( first_col < 0 )
        first_col = 0;
    int length = last_col - first_col + 1;
    if ( length > 0 ) {
        list << Chunk ( first_col, length, fore, back );
    }
}

void LineDrawer::addChunk( const Chunk& chunk)
{
    list << chunk;
}

void LineDrawer::addChunk( const LineChunk& chunk,
        QColor fore, QColor back )
{
    int first_col = chunk.start();
    int last_col  = chunk.end();

    addChunk( first_col, last_col, fore, back );
}

void LineDrawer::draw( QPainter& painter,
        int initialXPos, int initialYPos,
        int line_width, const QString& line,
        int leftExtraBackgroundPx )
{
    QFontMetrics fm = painter.fontMetrics();
    const int fontHeight = fm.height();
    const int fontAscent = fm.ascent();
    // For some reason on Qt 4.8.2 for Win, maxWidth() is wrong but the
    // following give the right result, not sure why:
    const int fontWidth = fm.width( QChar('a') );

    int xPos = initialXPos;
    int yPos = initialYPos;

    foreach ( Chunk chunk, list ) {
        // Draw each chunk
        // LOG(logDEBUG) << "Chunk: " << chunk.start() << " " << chunk.length();
        QString cutline = line.mid( chunk.start(), chunk.length() );
        const int chunk_width = cutline.length() * fontWidth;
        if ( xPos == initialXPos ) {
            // First chunk, we extend the left background a bit,
            // it looks prettier.
            painter.fillRect( xPos - leftExtraBackgroundPx, yPos,
                    chunk_width + leftExtraBackgroundPx,
                    fontHeight, chunk.backColor() );
        }
        else {
            // other chunks...
            painter.fillRect( xPos, yPos, chunk_width,
                    fontHeight, chunk.backColor() );
        }
        painter.setPen( chunk.foreColor() );
        painter.drawText( xPos, yPos + fontAscent, cutline );
        xPos += chunk_width;
    }

    // Draw the empty block at the end of the line
    int blank_width = line_width - xPos;

    if ( blank_width > 0 )
        painter.fillRect( xPos, yPos, blank_width, fontHeight, backColor_ );
}

