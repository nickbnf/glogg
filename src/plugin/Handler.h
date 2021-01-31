#ifndef BASETESTHANDLER_H
#define BASETESTHANDLER_H
#include <string>
#include <vector>
#include <map>
#include <typeinfo>
#include <iostream>
#include <memory>
#include <algorithm>
#include <boost/core/demangle.hpp>
#include <boost/python.hpp>

using namespace std;

class Handler
{
    public:
    Handler() = default;
    virtual ~Handler() = default;

    virtual void onRelease();
    virtual std::optional<boost::python::api::object> del() = 0;

    static Handler& getHandler(const string& className);

    const string& getInstanceName();
    vector<string> mUsingProperties;

protected:

    string className;
    string mInstanceName;
};




#endif // BASETESTHANDLER_H