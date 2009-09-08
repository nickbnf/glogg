#include "abstractlogdata.h"

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

