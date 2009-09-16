#include <iostream>

#include "log.h"

#include "logdata.h"

LogData::LogData(const QByteArray &byteArray) : AbstractLogData()
{
    list = new QStringList();
    int pos=0, end=0;
    while ( (end = byteArray.indexOf("\n", pos)) != -1 ) {
        const int length = end-pos;
        const QString string = QString(byteArray.mid(pos, length));
        if ( length > maxLength)
            maxLength = length;
        pos = end+1;
        list->append(string);
    }
    nbLines = list->length();

    LOG(logDEBUG) << "Found " << nbLines << " lines.";
}

int LogData::doGetNbLine() const
{
    return nbLines;
}

QString LogData::doGetLineString(int line) const
{
    QString string = list->at(line);

    return string;
}

LogFilteredData* LogData::getNewFilteredData(QRegExp& regExp) const
{
    LogFilteredData* newFilteredData = new LogFilteredData(list, regExp);

    return newFilteredData;
}
