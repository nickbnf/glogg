#include "abstractlogdata.h"

AbstractLogData::AbstractLogData()
{
    maxLength = 0;
}

// Simple wrapper in order to use a clean Template Method
QString AbstractLogData::getLineString(int line) const
{
    return doGetLineString(line);
}

// Simple wrapper in order to use a clean Template Method
int AbstractLogData::getNbLine() const
{
    return doGetNbLine();
}

int AbstractLogData::getMaxLength() const
{
    return maxLength;
}
