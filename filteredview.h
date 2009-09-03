#ifndef FILTEREDVIEW_H
#define FILTEREDVIEW_H

#include <QAbstractScrollArea>

#include "logfiltereddata.h"

class FilteredView : public QAbstractScrollArea
{
    Q_OBJECT

    public:
        FilteredView(QWidget *parent=0);

        void updateData(const LogFilteredData* newLogFileredData);

    protected:
        virtual void paintEvent(QPaintEvent* paintEvent);
        virtual void resizeEvent(QResizeEvent* resizeEvent);
        virtual void scrollContentsBy(int dx, int dy);

    private:
        const LogFilteredData* logFilteredData;
        int firstLine;
        int lastLine;

        int getNbVisibleLines();
};

#endif
