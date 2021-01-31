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
    PyHandlerInitParams(PyHandler* r):ref(r)
    {}
    PyHandler* ref;
};

class PyHandler : public Handler {
public:
    PyHandler();
    PyHandler(PyHandlerInitParams *init);
    virtual ~PyHandler();

    void setPyhonObject(const boost::python::object& obj);
    void setPythonPlugin(PythonPluginInterface* pp) {mPythonPlugin = pp;}
    bool setPyHandlerCallCount(string handler, int count);

    boost::optional<boost::python::api::object> del() override;

    boost::optional<boost::python::object> mObj;
    private:
    const char* on_popup_menu = "on_popup_menu";
    const char* on_create_menu = "on_create_menu";

    const char* on_trigger = "on_trigger";
    const char* on_release = "on_release";
    const char* on_search = "on_search";
    const char* on_show_ui = "on_show_ui";
    const char* on_display_line = "on_display_line";

    map<string, int> mPyHandlers = {{on_trigger, -1}};
    PythonPluginInterface *mPythonPlugin = nullptr;

    // Handler interface
    bool onTriggerAction(const string &action, const string &data);
public:
    bool onTrigger(int index);

    // Handler interface
public:
    static std::mutex pyContextLock;
    void onRelease() override;

    void onPopupMenu(AbstractLogView *alv);
    void onCreateMenu(AbstractLogView *alv);
    bool isOnSearcAvailable();
    SearchResultArray onSearch(const string &fileName, const string &pattern, int initialLine);
    bool doGetExpandedLines(string &line);
    void updateAppViews();
    void onShowUI();
};


#endif // MYMODULE_H
