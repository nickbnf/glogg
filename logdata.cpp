#include <iostream>

#include "log.h"

#include "logdata.h"

LogData::LogData(const QByteArray &byteArray) : AbstractLogData()
{
#ifdef LEGACY
    int j = 0;

    data = new QByteArray(byteArray);

    // Count the lines
    nbLines = 0;
    while (( j = data->indexOf("\n", j)) != -1) {
        nbLines++; j++;
    }
#elif defined(QLISTSTRING)
    list = new QStringList();
    int pos=0, end=0;
    while ( (end = byteArray.indexOf("\n", pos)) != -1 ) {
        const QString string = QString(byteArray.mid(pos, end-pos));
        pos = end+1;
        list->append(string);
    }
    nbLines = list->length();
#endif

    LOG(logDEBUG) << "Found " << nbLines << " lines.";
}

int LogData::doGetNbLine() const
{
    return nbLines;
}

QString LogData::doGetLineString(int line) const
{
#ifdef LEGACY
    int pos = 0;

    // Search the requested line
    for (int i = 0; i < line; i++, pos++)
        pos = data->indexOf("\n", pos);

    int end = data->indexOf("\n", pos);

    LOG(logDEBUG2) << "line " << line << " pos: " << pos << " end: " << end;

    QString string = QString(data->mid(pos, end-pos));

    LOG(logDEBUG2) << string.toStdString();
#elif defined(QLISTSTRING)
    QString string = list->at(line);
#endif

    return string;
}

LogFilteredData* LogData::getNewFilteredData(QRegExp& regExp) const
{
#ifdef LEGACY
    LogFilteredData* newObject = new LogFilteredData(data, regExp);

    return newObject;
#endif
}
