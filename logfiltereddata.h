#ifndef LOGFILTEREDDATA_H
#define LOGFILTEREDDATA_H

#include <QByteArray>
#include <QList>
#include <QRegExp>

#include "abstractlogdata.h"

class LogFilteredData : public AbstractLogData{
    public:
        LogFilteredData(QByteArray* logData, QRegExp regExp);

    private:
        QString doGetLineString(int line) const;
        int doGetNbLine() const;

        QList<QString> lineList;
};

#endif

