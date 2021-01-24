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
#include "pluginset.h"
#include <memory>
#include <functional>
#include <PythonPluginInterface.h>

using namespace std;

class PyHandler;
class AbstractLogView;


struct PyGIL {
  PyGILState_STATE state;
  PyGIL() {
    state = PyGILState_Ensure();
  }

  ~PyGIL() {
    PyGILState_Release(state);
  }
};

class PythonPlugin: public PythonPluginInterface
{
public:
    PythonPlugin();
    void createInstances();
    void onPopupMenu(AbstractLogView* alv);
    void onCreateMenu(AbstractLogView *alv);
    bool isOnSearcAvailable();
    SearchResultArray doSearch(const string &fileName, const string &pattern);
    void doGetExpandedLines(string &line);

    map<string, bool> getConfig() const;
    void setPluginState(const string& typeName, bool state);
    void enable(bool set);
    bool isEnabled() override;
    void registerUpdateViewsFunction(function<void ()> updateViewsFun) override;
    void updateAppViews() override;

private:

    struct PythonPluginImpl: public PythonPluginInterface
    {
        struct PythonGlobalInitialization
        {
            PythonGlobalInitialization()
            {
                Py_Initialize();
            }

            ~PythonGlobalInitialization()
            {
                Py_Finalize();
            }
        };

        PythonPluginImpl() = default;
        PythonPluginImpl(const map<string, bool>& config);
        ~PythonPluginImpl();
        void createInstance(std::optional<boost::python::object> type, const string &typeName);

        struct DerivedType
        {
            DerivedType() = default;
            ~DerivedType();
            DerivedType(const string& name_, boost::python::object& type_): name(name_), type(type_){}
            string name;
            boost::python::object type;
        };

        template<typename T>
        static shared_ptr<T> create(T* obj)
        {
            return shared_ptr<T>(obj, [](T* o){ o->del(); });
        }
        vector<DerivedType> mDerivedClassContainer;
        map<string, shared_ptr<PyHandler>> mHandlers;
        PyThreadState *threadState = nullptr;
        map<string, bool> mInitialConfig;
        function<void ()> mUpdateViewsFun;
        function<void (string, string, string, function<void (string)>)> mCreateAction;

        void onShowUI(string pluginName);

        // PythonPluginInterface interfaces
    public:
        void createInstances() override;
        void onPopupMenu(AbstractLogView *alv) override;
        void onCreateMenu(AbstractLogView *alv) override;
        bool isOnSearcAvailable() override;
        SearchResultArray doSearch(const string &fileName, const string &pattern) override;
        void doGetExpandedLines(string &line) override;
        map<string, bool> getConfig() const override;
        void setPluginState(const string &typeName, bool state) override;
        void registerUpdateViewsFunction(function<void ()> updateViewsFun) override;
        bool isEnabled() override;
        void enable(bool set) override;
        void updateAppViews() override;

        // PythonPluginInterface interface
    public:
        void onCreateToolBars(function<void (string, string, string, function<void (string)>)> createAction) override;
        void onCreateToolBarItem(string pluginName);
    };

    unique_ptr<PythonPluginImpl> mPluginImpl;

    // PythonPluginInterface interface

    // PythonPluginInterface interface
public:
    void onCreateToolBars(function<void (string, string, string, function<void (string)>)> createAction) override;
    void onCreateToolBarItem(string pluginName);
};





#endif // PYTHONPLUGIN_H