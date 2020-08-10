#include "PyHandler.h"

#include "PyHandler.h"
#include "boost/python/suite/indexing/vector_indexing_suite.hpp"

using namespace boost::python;

BOOST_PYTHON_MODULE(PyHandler)
{
    class_<std::vector<string> >("StringVec")
        .def(vector_indexing_suite<std::vector<string> >());

    class_<PyHandlerInitParams>("PyHandlerInitParams");

    //
    // https://www.mantidproject.org/Return_Value_Policies
    // return_value_policy<copy_const_reference>() - This will essentially make it return as if it were by value;
    // return_internal_reference() - This creates a Python object that contains a reference to the original C++ object.
    //
    class_<PyHandler>("PyHandler", init<PyHandlerInitParams*>())
        .def("seq_properties", &PyHandler::getRawProperties)
        .def("set_call_count", &PyHandler::setPyHandlerCallCount)
    ;
}


PyHandler::PyHandler()
{
    cout << __FUNCTION__ << " ctor \n";
}

PyHandler::PyHandler(PyHandlerInitParams *init)
{
    if(not init){
        return;
    }

    setPyhonType(init->ref->mType);
    setPythonPlugin(init->ref->mpyPlugin);
    className = init->ref->className;
    mInstanceName = init->ref->mInstanceName;

    setProperties(*init->prop);
}


bool PyHandler::onTrigger(int index)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    bool ret = false;
    if(mPyHandlers[on_trigger] != 0 &&
            PyObject_HasAttrString(mObj.ptr(), on_trigger)){

        if (mPyHandlers[on_trigger] > 0){
            --mPyHandlers[on_trigger];
        }

//        cout << __FUNCTION__ << " call python, index: " << index << "\n";
        ret = mObj.attr(on_trigger)(index);
    }

    PyGILState_Release(gstate);

    return ret;
}

void PyHandler::onPopupMenu(AbstractLogView* alv)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    if(PyObject_HasAttrString(mObj.ptr(), on_popup_menu)){

//        cout << __FUNCTION__ << " call python, index: " << index << "\n";
        mObj.attr(on_popup_menu)();
    }

    PyGILState_Release(gstate);
}

bool PyHandler::onTriggerAction(const string& action, const string& data)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    bool ret = false;
    if(PyObject_HasAttrString(mObj.ptr(), action.c_str())){

        ret = mObj.attr(action.c_str())(data);
    } else{
        throw std::logic_error(string("\n!ERROR! [") +
                               __FUNCTION__ +
                               string("] trigger action:[" + action + "] Not found during executing of Python code\n"));
    }

    PyGILState_Release(gstate);

    return ret;
}

void PyHandler::setPyhonType(const boost::python::api::object &type)
{
    mType = type;
}

void PyHandler::setPyhonObject(const boost::python::api::object &obj)
{
    mObj = obj;
}


bool PyHandler::setPyHandlerCallCount(string handler, int count)
{
     cout << "\n" << __FUNCTION__ << " handler:"  << handler << " count:" << count << "\n";

    try{
        mPyHandlers.at(handler) = count;
    }catch(const exception&){
        cout << "\n" << __FUNCTION__ << " NOT registered !" << "\n";
        return false;
    }

    return true;
}

void PyHandler::onRelease()
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    try{
        if(PyObject_HasAttrString(mObj.ptr(), on_release)){
            mObj.attr(on_release)();
        }
    } catch (error_already_set& e) {
        PyErr_PrintEx(0);
        throw std::logic_error(string("\n[") +
                               __FUNCTION__ +
                               string("] !Error during executing Python code\n"));
    }

    PyGILState_Release(gstate);
}
