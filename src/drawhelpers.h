#ifndef DRAWHELPERS_H
#define DRAWHELPERS_H

#include <QColor>
#include <QList>
#include <QPainter>

class LineChunk
{
  public:
    enum ChunkType {
        Normal,
        Highlighted,
        Selected,
        Filtered
    };

    LineChunk( int first_col, int end_col, ChunkType type );
    LineChunk( int first_col, int end_col, QColor fore, QColor back ):
        start_(first_col), end_(end_col), type_(Filtered), foreground_(fore), background_(back)
    {}

    int start() const { return start_; }
    int end() const { return end_; }
    ChunkType type() const { return type_; }
    QColor foreground() const { return foreground_; }
    QColor background() const { return background_; }

    // Returns 'true' if the selection is part of this chunk
    // (at least partially), if so, it should be replaced by the list returned
    QList<LineChunk> select( int selection_start, int selection_end ) const;

    QList<LineChunk> selectFiltered(LineChunk chunk) const;
private:
    int start_;
    int end_;
    ChunkType type_;
    QColor foreground_;
    QColor background_;
};

// Utility class for syntax colouring.
// It stores the chunks of line to draw
// each chunk having a different colour
class LineDrawer
{
  public:
    class Chunk {
      public:
        // Create a new chunk
        Chunk( int start, int length, QColor fore, QColor back )
            : start_( start ), length_( length ),
            foreColor_ ( fore ), backColor_ ( back ) { }

        Chunk( int start, int length, const QString& fore, const QString& back )
            : start_( start ), length_( length ),
            foreColor_ ( fore ), backColor_ ( back ) { }

        // Accessors
        int start() const { return start_; }
        int length() const { return length_; }
        const QColor& foreColor() const { return foreColor_; }
        const QColor& backColor() const { return backColor_; }

      private:
        int start_;
        int length_;
        QColor foreColor_;
        QColor backColor_;
    };

    LineDrawer( const QColor& back_color) :
        list(), backColor_( back_color ) { }

    // Add a chunk of line using the given colours.
    // Both first_col and last_col are included
    // An empty chunk will be ignored.
    // the first column will be set to 0 if negative
    // The column are relative to the screen
    void addChunk( int first_col, int last_col, QColor fore, QColor back );
    void addChunk( const LineChunk& chunk, QColor fore, QColor back );
    void addChunk( const Chunk& chunk);

    // Draw the current line of text using the given painter,
    // in the passed block (in pixels)
    // The line must be cut to fit on the screen.
    // leftExtraBackgroundPx is the an extra margin to start drawing
    // the coloured // background, going all the way to the element
    // left of the line looks better.
    void draw( QPainter& painter, int xPos, int yPos,
               int line_width, const QString& line,
               int leftExtraBackgroundPx );

private:
    QList<Chunk> list;
    QColor backColor_;
};

#endif // DRAWHELPERS_H
