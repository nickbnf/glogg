#include <QString>

#include "logfiltereddata.h"

LogFilteredData::LogFilteredData(QByteArray* logData, QRegExp regExp)
{
}

QString LogFilteredData::getLineString(int line) const
{
    QString string(line);

    return string;
}

int LogFilteredData::getNbLine() const
{
    return 4;
}
