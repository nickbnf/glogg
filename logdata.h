#ifndef LOGDATA_H
#define LOGDATA_H

#include <QByteArray>
#include <QString>

#include "logfiltereddata.h"

class LogData {
    public:
        LogData(const QByteArray &byteArray);
        QString getLineString(int line);
        int getNbLine();
        LogFilteredData getNewFilteredView(QRegExp& regExp);

    private:
        QByteArray* data;
        int nbLines;
};

#endif
