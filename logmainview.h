#ifndef LOGMAINVIEW_H
#define LOGMAINVIEW_H

#include <QAbstractScrollArea>

#include "logdata.h"

class LogMainView : public QAbstractScrollArea
{
    Q_OBJECT

    public:
        LogMainView(QWidget *parent=0);

        bool readFile(const QString &fileName);

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
