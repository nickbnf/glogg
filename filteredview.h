#ifndef FILTEREDVIEW_H
#define FILTEREDVIEW_H

#include <QAbstractScrollArea>

#include "logdata.h"

class FilteredView : public QAbstractScrollArea
{
    Q_OBJECT

    public:
        FilteredView(QWidget *parent=0);

    protected:
        virtual void paintEvent(QPaintEvent* paintEvent);
        virtual void resizeEvent(QResizeEvent* resizeEvent);
        virtual void scrollContentsBy(int dx, int dy);

    private:
        LogData* logData;
        int firstLine;
        int lastLine;

        int getNbVisibleLines();
};

#endif
