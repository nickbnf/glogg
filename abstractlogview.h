#ifndef ABSTRACTLOGVIEW_H
#define ABSTRACTLOGVIEW_H

#include <QAbstractScrollArea>

#include "abstractlogdata.h"

/**
 * @brief Base class representing the log view widget.
 *
 * It can be either the top (full) or bottom (filtered) view.
 */
class AbstractLogView : public QAbstractScrollArea
{
    Q_OBJECT

    public:
        AbstractLogView(QWidget *parent=0);

        void updateData(const AbstractLogData* newLogData);

    protected:
        void mousePressEvent( QMouseEvent* mouseEvent );
        void paintEvent(QPaintEvent* paintEvent);
        void resizeEvent(QResizeEvent* resizeEvent);
        void scrollContentsBy(int dx, int dy);

    signals:
        void newSelection( int line );

    public slots:
        void displayLine( int line );

    private:
        const AbstractLogData* logData;
        int firstLine;
        int lastLine;
        /// @brief Line number currently selected, or -1 if none selected
        int selectedLine;

        int getNbVisibleLines();
        int convertCoordToLine(int yPos);
};

#endif
