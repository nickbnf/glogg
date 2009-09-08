#ifndef LOGDATA_H
#define LOGDATA_H

#include <QByteArray>
#include <QString>

#include "abstractlogdata.h"
#include "logfiltereddata.h"

class LogData : public AbstractLogData {
    public:
        LogData(const QByteArray& byteArray);

        LogFilteredData* getNewFilteredData(QRegExp& regExp) const;

    private:
        QString doGetLineString(int line) const;
        int doGetNbLine() const;

        QByteArray* data;
        int nbLines;
};

#endif
