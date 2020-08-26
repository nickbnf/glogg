#include "PythonPlugin.h"
#include "PyHandler.h"
#include <iostream>
#include <boost/filesystem.hpp>
//#include "Log.h"
#include "PyHandler.h"
#include "abstractlogview.h"

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

    try {
        PyImport_AppendInittab((char*)"PyHandler", INIT_MODULE);
        Py_Initialize();
        //PyEval_InitThreads();

        boost::filesystem::path workingDir = boost::filesystem::absolute("").normalize();

        string s= workingDir.string();

        PyObject* sysPath = PySys_GetObject("path");
        PyList_Insert( sysPath, 0, PyUnicode_FromString(workingDir.string().c_str()));

        object main_module = import("__main__");
        dict main_namespace = extract<dict>(main_module.attr("__dict__"));

        object mymodule = import("PyHandler");
        dict mymodule_namespace = extract<dict>(mymodule.attr("__dict__"));
        object BaseClass = mymodule_namespace["PyHandler"];
        //object VerifierBaseClass = mymodule_namespace["PyTestVerifier"];

        exec_file("handlers.py", main_namespace, main_namespace);
        exec_file("handlers2.py", main_namespace, main_namespace);
        exec_file("PyDialog.py", main_namespace, main_namespace);
        PyTypeObject* base_class = reinterpret_cast<PyTypeObject*>(BaseClass.ptr());
        //PyTypeObject* verifier_base_class = reinterpret_cast<PyTypeObject*>(VerifierBaseClass.ptr());

        boost::python::list keys = main_namespace.keys();

        for (unsigned int i = 0; i<len(keys) ; ++i) {
            object k = keys[i];

            object item = main_namespace[k];

            PyObject* item_ptr = item.ptr();

            if ( PyType_Check(item_ptr) != 0 ) {
                PyTypeObject* type_obj = reinterpret_cast<PyTypeObject*>(item_ptr);

                if ( ( type_obj != base_class) && ( PyType_IsSubtype( type_obj,  base_class) > 0) ) {
                    cout << "type name:" << type_obj->tp_name << "\n";
                    mDerivedClassContainer.emplace_back(DerivedType(type_obj->tp_name, item));
                }/*else if(( type_obj != verifier_base_class) && ( PyType_IsSubtype( type_obj,  verifier_base_class) > 0)){
                    cout << "verifier type name:" << type_obj->tp_name << "\n";
                    mDerivedVerifierClassContainer.emplace(type_obj->tp_name, item);
                }*/
            }
        }

    } catch (error_already_set& e) {
        PyErr_PrintEx(0);
        throw std::logic_error("!Error during loading PythonPlugin\n");
    }

    //threadState = PyEval_SaveThread();
}

void PythonPlugin::createInstances()
{
    string typeName;

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyHandlerInitParams *init = nullptr;

    try {
        for(auto& t: mDerivedClassContainer){
            typeName = t.name;
            boost::python::object obj = t.type(init);
            Py_INCREF(obj.ptr());

            PyHandler* p = extract<PyHandler*>(obj.ptr());
            p->setPyhonObject(obj);
            p->setPythonPlugin(this);

            p->setPyhonType(t.type);

            mHandlers.emplace(typeName, create(p));
            //Handler::addPythonTemplate<PyHandler>(handlers, t.name, p);
        }

    } catch (error_already_set& e) {
        PyErr_PrintEx(0);
        throw std::logic_error("\n!Error while loading template handler: [" + typeName + "]\n\n");
    }

    PyGILState_Release(gstate);
}

shared_ptr<PyHandler> PythonPlugin::operator [](const string &className)
{
    return mHandlers.at(className);
}

void PythonPlugin::setActivePlugin(const string &className)
{
    mActiveHandler = mHandlers.at(className);
}

void PythonPlugin::onPopupMenu(AbstractLogView *alv)
{
    for(auto &o: mHandlers){
        o.second->onPopupMenu(alv);
    }
}

void PythonPlugin::onCreateMenu(AbstractLogView *alv)
{
    for(auto &o: mHandlers){
        o.second->onCreateMenu(alv);
    }
}

bool PythonPlugin::isOnSearcAvailable()
{
    for(auto &o: mHandlers){
        if (o.second->isOnSearcAvailable()){
            return true;
        }
    }

    return false;
}

SearchResultArray PythonPlugin::doSearch(const string& fileName, const string& pattern )
{
    for(auto &o: mHandlers){
        if (o.second->isOnSearcAvailable()){
            return o.second->onSearch(fileName, pattern);
        }
    }

    return {};
}

void PythonPlugin::doGetExpandedLines(string& line)
{
    for(auto &o: mHandlers){
//        if (o.second->isOnSearcAvailable()){
            return o.second->doGetExpandedLines(line);
//        }
    }

}


//boost::python::api::object PythonPlugin::getTestVerifierType(const string &className)
//{
//    return mDerivedVerifierClassContainer.at(className);
//}
