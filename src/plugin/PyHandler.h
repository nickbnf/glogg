#ifndef MYMODULE_H
#define MYMODULE_H

#include <string>
#include <boost/python.hpp>
#include "Handler.h"
#include "PythonPlugin.h"
#include <mutex>

class PyHandler;

struct PyHandlerInitParams
{
    PyHandlerInitParams(){}
    PyHandlerInitParams(PyHandler* r, JSONParser* p, int i):ref(r), prop(p), inpuFD(i)
    {}
    PyHandler* ref;
    JSONParser* prop;
    int inpuFD;
};

class PyHandler : public Handler {
public:
    PyHandler();
    PyHandler(PyHandlerInitParams *init);
    virtual ~PyHandler() {}

    void setPythonPlugin(PythonPlugin* pyPlugin) { mpyPlugin = pyPlugin; }
    void setPyhonType(const boost::python::object& type);
    void setPyhonObject(const boost::python::object& obj);
    bool setPyHandlerCallCount(string handler, int count);

    private:
    const char* on_popup_menu = "on_popup_menu";
    const char* on_create_menu = "on_create_menu";

    const char* on_trigger = "on_trigger";
    const char* on_release = "on_release";

    map<string, int> mPyHandlers = {{on_trigger, -1}};
    boost::python::object mType;
    boost::python::object mObj;

    // Handler interface
    bool onTriggerAction(const string &action, const string &data);
public:
    bool onTrigger(int index);
    PythonPlugin* mpyPlugin = nullptr;

    // Handler interface
public:
    void onRelease() override;

    void onPopupMenu(AbstractLogView *alv);
    void onCreateMenu(AbstractLogView *alv);
};


#endif // MYMODULE_H