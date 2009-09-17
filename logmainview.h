#ifndef LOGMAINVIEW_H
#define LOGMAINVIEW_H

#include "abstractlogview.h"

class LogMainView : public AbstractLogView
{
    public:
        LogMainView(const AbstractLogData* newLogData, QWidget* parent = 0);
};

#endif
