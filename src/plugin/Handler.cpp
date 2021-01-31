#include "Handler.h"
#include <iostream>

void Handler::onRelease()
{
    cout << "\n" << __FUNCTION__ << "\n";
}

const string &Handler::getInstanceName()
{
    return mInstanceName;
}
