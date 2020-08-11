#ifndef PYTHONPLUGIN_H
#define PYTHONPLUGIN_H

#include <vector>
#include <map>
#pragma push_macro("slots")
#undef slots
//#undef override
#include <boost/python.hpp>
#pragma pop_macro("slots")
#include <string>
#include <JSonParser.h>
#include "Handler.h"
#include "data/search_result.h"

using namespace std;

class PyHandler;
class AbstractLogView;


class PythonPlugin
{
public:
    PythonPlugin();
    //const std::unordered_map<string, PyTypeObject*>& getTypeMap() const;
    void createInstances();
    shared_ptr<PyHandler> operator [](const string& className);
    void setActivePlugin(const string& className);
    void onPopupMenu(AbstractLogView* alv);
    //boost::python::object getTestVerifierType(const string& className);
    void onCreateMenu(AbstractLogView *alv);
    bool isOnSearcAvailable();
    SearchResultArray doSearch(const string &fileName, const string &pattern);
private:
    struct DerivedType
    {
        DerivedType() = default;
        DerivedType(const string& name_, boost::python::object type_): name(name_), type(type_){}
        string name;
        boost::python::object type;
        //vector<boost::python::object> instanceContainer;
//        boost::python::object instance;
    };

    template<typename T>
    static shared_ptr<T> create(T* obj)
    {
        return shared_ptr<T>(obj, [](Handler* o){});
    }

    vector<DerivedType> mDerivedClassContainer;
    map<string, shared_ptr<PyHandler>> mHandlers;
    //map<string, boost::python::object> mDerivedVerifierClassContainer;
    //map<string, boost::python::object> mDerivedClassMap;
    shared_ptr<PyHandler> mActiveHandler;

    PyThreadState *threadState = nullptr;

};

#endif // PYTHONPLUGIN_H