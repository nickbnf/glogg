#include "Handler.h"
#include <iostream>

void Handler::onRelease()
{
    cout << "\n" << __FUNCTION__ << "\n";
}

void Handler::loadFromJson(const JSONParser &jp)
{
    vector<string> usingProperties = jp.getArrayOfValues<string>("using");

    for(const auto &tr: usingProperties){
        cout << "[" << className << "]" << " Using property: [" << tr << "]\n";
        mUsingProperties.push_back(tr);
    }
}

void Handler::setProperties(const JSONParser &properties)
{
    mProperties = properties;
}

const string &Handler::getInstanceName()
{
    return mInstanceName;
}
