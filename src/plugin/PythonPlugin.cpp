#include "PythonPlugin.h"
#include "PyHandler.h"
#include <iostream>
#include <boost/filesystem.hpp>
//#include "Log.h"
#include "PyHandler.h"
#include "abstractlogview.h"
#include "persistentinfo.h"

using namespace boost::python;

#if PY_MAJOR_VERSION >= 3
#   define INIT_MODULE PyInit_PyHandler
    extern "C" PyObject* INIT_MODULE();
#else
#   define INIT_MODULE initmymodule
    extern "C" void INIT_MODULE();
#endif


PythonPlugin::PythonPlugin()
{
    using namespace boost::python;

    GetPersistentInfo().retrieve("pluginSet");
    enable(Persistent<PluginSet>( "pluginSet" )->isPluginSystemEnabled());
}

void PythonPlugin::createInstances()
{
    if(not mPluginImpl){
        return;
    }

    mPluginImpl->createInstances();
}

void PythonPlugin::onPopupMenu(AbstractLogView *alv)
{
    if(not mPluginImpl){
        return;
    }

    mPluginImpl->onPopupMenu(alv);
}

void PythonPlugin::onCreateMenu(AbstractLogView *alv)
{
    if(not mPluginImpl){
        return;
    }

    mPluginImpl->onCreateMenu(alv);
}

bool PythonPlugin::isOnSearcAvailable()
{
    if(not mPluginImpl){
        return false;
    }

    return mPluginImpl->isOnSearcAvailable();
}

SearchResultArray PythonPlugin::doSearch(const string& fileName, const string& pattern )
{
    if(not mPluginImpl){
        return {};
    }

    return mPluginImpl->doSearch(fileName, pattern);
}

void PythonPlugin::doGetExpandedLines(string& line)
{
    if(not mPluginImpl){
        return;
    }

    mPluginImpl->doGetExpandedLines(line);
}

map<string, bool> PythonPlugin::getConfig() const
{
    if(not mPluginImpl){
        return {};
    }

    return mPluginImpl->getConfig();
}

void PythonPlugin::setPluginState(const string &typeName, bool state)
{
    if(not mPluginImpl){
        return;
    }

    mPluginImpl->setPluginState(typeName, state);
}

void PythonPlugin::enable(bool set)
{
    if(set){
        GetPersistentInfo().retrieve("pluginSet");

        mPluginImpl = make_unique<PythonPluginImpl>(Persistent<PluginSet>( "pluginSet" )->getPlugins());
    }else{
        mPluginImpl.reset();
    }

}

bool PythonPlugin::isEnabled()
{
    return mPluginImpl.operator bool();
}

void PythonPlugin::registerUpdateViewsFunction(function<void ()> updateViewsFun)
{
    if(not mPluginImpl){
        return;
    }

    mPluginImpl->registerUpdateViewsFunction(updateViewsFun);
}

void PythonPlugin::updateAppViews()
{
    mPluginImpl->updateAppViews();
}

void PythonPlugin::PythonPluginImpl::enable(bool set)
{
}

bool PythonPlugin::PythonPluginImpl::isEnabled()
{
    return true;
}


void PythonPlugin::PythonPluginImpl::registerUpdateViewsFunction(function<void ()> updateViewsFun)
{
    mUpdateViewsFun = updateViewsFun;
}

vector<string> getPythonPluginFilePaths(const string dir)
{
    vector<string> ret;

    cout << "Looking for python plugins files:\n";

    for(auto & entry : boost::filesystem::directory_iterator( dir )){
        if(entry.path().extension() == ".py"){
            std::cout << entry << std::endl;
            ret.push_back(entry.path().string());
        }
    }

    return ret;
}

PythonPlugin::PythonPluginImpl::PythonPluginImpl(const map<string, bool> &config):mInitialConfig(config)
{
    using namespace boost::python;

    try {
        PyImport_AppendInittab((char*)"PyHandler", INIT_MODULE);
        Py_Initialize();
        PyEval_InitThreads();

        boost::filesystem::path workingDir = boost::filesystem::absolute("").normalize();

        string s= workingDir.string() + "/plugins";
        cout << "WorkDir: " << s << "\n";


        PyObject* sysPath = PySys_GetObject("path");
        PyList_Insert( sysPath, 0, PyUnicode_FromString(/*workingDir.string()*/s.c_str()));

        object main_module = import("__main__");
        dict main_namespace = extract<dict>(main_module.attr("__dict__"));

        object mymodule = import("PyHandler");
        dict mymodule_namespace = extract<dict>(mymodule.attr("__dict__"));
        object BaseClass = mymodule_namespace["PyHandler"];
        //object VerifierBaseClass = mymodule_namespace["PyTestVerifier"];

        cout <<"\n";
        for(const auto file: getPythonPluginFilePaths(s)){
            cout << "Loading plugin file to python namespace: " << file << "\n";
            exec_file(file.c_str(), main_namespace, main_namespace);
        }

        PyTypeObject* base_class = reinterpret_cast<PyTypeObject*>(BaseClass.ptr());
        //PyTypeObject* verifier_base_class = reinterpret_cast<PyTypeObject*>(VerifierBaseClass.ptr());

        boost::python::list keys = main_namespace.keys();

        cout << "\nLooking for types derived from main plugin type:\n";

        for (unsigned int i = 0; i<len(keys) ; ++i) {
            object k = keys[i];

            object item = main_namespace[k];

            PyObject* item_ptr = item.ptr();

            if ( PyType_Check(item_ptr) != 0 ) {
                PyTypeObject* type_obj = reinterpret_cast<PyTypeObject*>(item_ptr);

                if ( ( type_obj != base_class) && ( PyType_IsSubtype( type_obj,  base_class) > 0) ) {
                    cout << "type name:" << type_obj->tp_name << "\n";
                    mDerivedClassContainer.emplace_back(DerivedType(type_obj->tp_name, item));
                }
            }
        }

    } catch (error_already_set& e) {
        PyErr_PrintEx(0);
        throw std::logic_error("!Error during loading PythonPlugin\n");
    }

    threadState = PyEval_SaveThread();

}

PythonPlugin::PythonPluginImpl::~PythonPluginImpl()
{
    mHandlers.clear();

//    shared_ptr<PyHandler> el = mHandlers["UI"];
//    mHandlers.erase("UI");
//    std::optional<boost::python::object> o = el->del();
//    o.reset();
    //Py_Finalize();
}

void PythonPlugin::PythonPluginImpl::createInstance(std::optional<boost::python::api::object> type, const string &typeName)
{
    PyGIL gil;

    if(not type){
        type = std::find_if(mDerivedClassContainer.begin(), mDerivedClassContainer.end(), [typeName](const DerivedType& t){ return t.name == typeName; })->type;
    }

    cout << "Creating plugin instance of type:" << typeName << "\n";

    PyHandlerInitParams *init = nullptr;


    boost::python::object obj = (*type)(init);

    PyHandler* p = extract<PyHandler*>(obj.ptr());
    p->setPyhonObject(obj);
    p->setPythonPlugin(this);

    mHandlers.emplace(typeName, create(p));

}

void PythonPlugin::PythonPluginImpl::updateAppViews()
{
    mUpdateViewsFun();
    cout << __FUNCTION__ <<"\n";
}

void PythonPlugin::PythonPluginImpl::createInstances()
{
    string typeName;

    PyGIL gil;

    cout << "\nCreating plugins instances:\n";

    try {
        for(auto& t: mDerivedClassContainer){
            if(mInitialConfig[t.name]){
                createInstance(t.type, t.name);
            }
        }

    } catch (error_already_set& e) {
        PyErr_PrintEx(0);
        throw std::logic_error("\n!Error while loading template handler: [" + typeName + "]\n\n");
    }

}

void PythonPlugin::PythonPluginImpl::onPopupMenu(AbstractLogView *alv)
{
    PyGIL gil;

    for(auto &o: mHandlers){
        o.second->onPopupMenu(alv);
    }
}

void PythonPlugin::PythonPluginImpl::onCreateMenu(AbstractLogView *alv)
{
    PyGIL gil;

    for(auto &o: mHandlers){
        o.second->onCreateMenu(alv);
    }
}

bool PythonPlugin::PythonPluginImpl::isOnSearcAvailable()
{
    PyGIL gil;

    for(auto &o: mHandlers){
        if (o.second->isOnSearcAvailable()){
            return true;
        }
    }

    return false;
}

SearchResultArray PythonPlugin::PythonPluginImpl::doSearch(const string &fileName, const string &pattern)
{
    PyGIL gil;

    for(auto &o: mHandlers){
        if (o.second->isOnSearcAvailable()){
            return o.second->onSearch(fileName, pattern);
        }
    }

    return {};
}

void PythonPlugin::PythonPluginImpl::doGetExpandedLines(string &line)
{
    PyGIL gil;

    for(auto &o: mHandlers){
//        if (o.second->isOnSearcAvailable()){
            if(o.second->doGetExpandedLines(line)){
                return;
            }
//        }
    }
}

map<string, bool> PythonPlugin::PythonPluginImpl::getConfig() const
{
    map<string, bool> config;

    for(auto& t: mDerivedClassContainer){
        config[t.name] = mHandlers.find(t.name) != mHandlers.end();
    }

    return config;
}

void PythonPlugin::PythonPluginImpl::setPluginState(const string &typeName, bool state)
{
    PyGIL gil;

    if(not state){
        //mHandlers[typeName]->onRelease();
        mHandlers.erase(typeName);
    }else{
        createInstance({}, typeName);
    }
}
PythonPlugin::PythonPluginImpl::DerivedType::~DerivedType()
{

}

