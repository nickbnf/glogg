#ifndef LOGDATA_H
#define LOGDATA_H

#include <QByteArray>
#include <QString>
#include <QStringList>

#include "abstractlogdata.h"
#include "logfiltereddata.h"

//#define LEGACY
#define QLISTSTRING

class LogData : public AbstractLogData {
    public:
        LogData(const QByteArray& byteArray);

        LogFilteredData* getNewFilteredData(QRegExp& regExp) const;

    private:
        QString doGetLineString(int line) const;
        int doGetNbLine() const;

#ifdef LEGACY
        QByteArray* data;
#elif defined(QLISTSTRING)
        QStringList* list;
#endif
        int nbLines;
};

#endif
