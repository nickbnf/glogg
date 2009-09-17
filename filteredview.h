#ifndef FILTEREDVIEW_H
#define FILTEREDVIEW_H

#include "abstractlogview.h"

class FilteredView : public AbstractLogView
{
    public:
        FilteredView(const AbstractLogData* newLogData, QWidget* parent = 0);
};

#endif
