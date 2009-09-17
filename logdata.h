#ifndef LOGDATA_H
#define LOGDATA_H

#include <QByteArray>
#include <QString>
#include <QStringList>

#include "abstractlogdata.h"
#include "logfiltereddata.h"

class LogData : public AbstractLogData {
    public:
        /// @brief Create an empty LogData
        LogData();
        /// @brief Create a log data from the data chunk passed
        LogData(const QByteArray& byteArray);

        LogFilteredData* getNewFilteredData(QRegExp& regExp) const;

    private:
        QString doGetLineString(int line) const;
        int doGetNbLine() const;

        QStringList* list;
        int nbLines;
};

#endif
