#ifndef LOGMAINVIEW_H
#define LOGMAINVIEW_H

#include <QWidget>

#include "logdata.h"

class LogMainView : public QWidget
{
    Q_OBJECT

    public:
        LogMainView(QWidget *parent=0);

        bool readFile(const QString &fileName);

    protected:
        virtual void paintEvent(QPaintEvent* paintEvent);

    private:
        LogData* logData;
        int firstLine;
        int lastLine;

        int getNbVisibleLines();
};

#endif
