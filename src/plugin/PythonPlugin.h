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

class PythonPluginInterface
{
public:
    virtual ~PythonPluginInterface() = default;

    virtual void createInstances() = 0;
    virtual void onPopupMenu(AbstractLogView* alv) = 0;
    virtual void onCreateMenu(AbstractLogView *alv) = 0;
    virtual bool isOnSearcAvailable() = 0;
    virtual SearchResultArray doSearch(const string &fileName, const string &pattern) = 0;
    virtual void doGetExpandedLines(string &line) = 0;
    virtual map<string, bool> getConfig() const = 0;
    virtual void setPluginState(const string& typeName, bool state) = 0;
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
    bool isEnabled();

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
    };

    unique_ptr<PythonPluginImpl> mPluginImpl;
};

#endif // PYTHONPLUGIN_H