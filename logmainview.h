#ifndef LOGMAINVIEW_H
#define LOGMAINVIEW_H

#include <QAbstractScrollArea>

#include "logdata.h"

class LogMainView : public QAbstractScrollArea
{
    Q_OBJECT

    public:
        LogMainView(QWidget *parent=0);

        void updateData(const LogData* newLogData);

    protected:
        virtual void paintEvent(QPaintEvent* paintEvent);
        virtual void resizeEvent(QResizeEvent* resizeEvent);
        virtual void scrollContentsBy(int dx, int dy);

    private:
        const LogData* logData;
        int firstLine;
        int lastLine;

        int getNbVisibleLines();
};

#endif
