#include <QString>

#include "logfiltereddata.h"

LogFilteredData::LogFilteredData(QByteArray* logData, QRegExp regExp) : AbstractLogData()
{
}

QString LogFilteredData::doGetLineString(int line) const
{
    QString string(line);

    return string;
}

int LogFilteredData::doGetNbLine() const
{
    return 4;
}
