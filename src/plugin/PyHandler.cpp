#include "PyHandler.h"

#include "PyHandler.h"
#include "boost/python/suite/indexing/vector_indexing_suite.hpp"

std::mutex PyHandler::pyContextLock;

using namespace boost::python;

BOOST_PYTHON_MODULE(PyHandler)
{
    class_<std::vector<string> >("StringVec")
        .def(vector_indexing_suite<std::vector<string> >());

    class_<std::vector<MatchingLine> >("SearchResultVec")
        .def(vector_indexing_suite<std::vector<MatchingLine> >());


    class_<PyHandlerInitParams>("PyHandlerInitParams");

    //
    // https://www.mantidproject.org/Return_Value_Policies
    // return_value_policy<copy_const_reference>() - This will essentially make it return as if it were by value;
    // return_internal_reference() - This creates a Python object that contains a reference to the original C++ object.
    //
    class_<PyHandler>("PyHandler", init<PyHandlerInitParams*>())
        .def("seq_properties", &PyHandler::getRawProperties)
        .def("set_call_count", &PyHandler::setPyHandlerCallCount)
        .def("update_app_views", &PyHandler::updateAppViews)
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

    className = init->ref->className;
    mInstanceName = init->ref->mInstanceName;

    setProperties(*init->prop);
}

PyHandler::~PyHandler()
{
}


bool PyHandler::onTrigger(int index)
{
    bool ret = false;
    if(mPyHandlers[on_trigger] != 0 &&
            PyObject_HasAttrString(mObj->ptr(), on_trigger)){

        if (mPyHandlers[on_trigger] > 0){
            --mPyHandlers[on_trigger];
        }

//        cout << __FUNCTION__ << " call python, index: " << index << "\n";
        ret = mObj->attr(on_trigger)(index);
    }

    return ret;
}

void PyHandler::onPopupMenu(AbstractLogView* alv)
{
    auto ul = std::unique_lock<std::mutex>(pyContextLock);

    if(PyObject_HasAttrString(mObj->ptr(), on_popup_menu)){

//        cout << __FUNCTION__ << " call python, index: " << index << "\n";
        mObj->attr(on_popup_menu)();
    }
}

void PyHandler::onCreateMenu(AbstractLogView* alv)
{
    std::unique_lock<std::mutex>(pyContextLock);

    if(PyObject_HasAttrString(mObj->ptr(), on_create_menu)){

//        cout << __FUNCTION__ << " call python, index: " << index << "\n";
        mObj->attr(on_create_menu)();
    }

}

bool PyHandler::onTriggerAction(const string& action, const string& data)
{
    std::unique_lock<std::mutex>(pyContextLock);

    bool ret = false;
    if(PyObject_HasAttrString(mObj->ptr(), action.c_str())){

        ret = mObj->attr(action.c_str())(data);
    } else{
        throw std::logic_error(string("\n!ERROR! [") +
                               __FUNCTION__ +
                               string("] trigger action:[" + action + "] Not found during executing of Python code\n"));
    }

    return ret;
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

std::optional<boost::python::object> PyHandler::del()
{
    //std::optional<boost::python::object> safeDelete = mObj;
    onRelease();
    std::optional<boost::python::object> ret = *mObj;

    mObj.reset();

    return ret;
}

void PyHandler::onRelease()
{
    try{
        if(PyObject_HasAttrString(mObj->ptr(), on_release)){
            mObj->attr(on_release)();
        }
    } catch (error_already_set& e) {
        PyErr_PrintEx(0);
        throw std::logic_error(string("\n[") +
                               __FUNCTION__ +
                               string("] !Error during executing Python code\n"));
    }
}


bool PyHandler::isOnSearcAvailable()
{
    std::unique_lock<std::mutex>(pyContextLock);

    try{
        if(PyObject_HasAttrString(mObj->ptr(), on_search)){
            return true;
        }
    } catch (error_already_set& e) {
        PyErr_PrintEx(0);
        throw std::logic_error(string("\n[") +
                               __FUNCTION__ +
                               string("] !Error during executing Python code\n"));
    }

    return false;
}

void PyHandler::onShowUI()
{
    std::unique_lock<std::mutex>(pyContextLock);

    try{
        if(PyObject_HasAttrString(mObj->ptr(), on_show_ui)){
            mObj->attr(on_show_ui)();
        }
    } catch (error_already_set& e) {
        PyErr_PrintEx(0);
        throw std::logic_error(string("\n[") +
                               __FUNCTION__ +
                               string("] !Error during executing Python code\n"));
    }
}

SearchResultArray PyHandler::onSearch(const string& fileName, const string& pattern, int initialLine)
{
    std::unique_lock<std::mutex>(pyContextLock);

    try{
        if(PyObject_HasAttrString(mObj->ptr(), on_search)){
            vector<string> ss;
            boost::python::object o = mObj->attr(on_search)(fileName.c_str(), pattern.c_str(), initialLine);
            list l = extract<boost::python::list>(o);
            cout << __FUNCTION__ << " "<< boost::python::len(l) << "\n";

            SearchResultArray ret;
            for(int i = 0; i < boost::python::len(l); ++i){
                char *data = extract<char*>(l[i]);
                //cout << __FUNCTION__ << "data " << data << "\n";
                if(strlen(data) > 0){
                    int index = atoi(data) - 1;
                    if(index > -1){
                        ret.push_back(MatchingLine(index));
                    }
                }
            }

            return ret;
        }
    } catch (error_already_set& e) {
        PyErr_PrintEx(0);
        throw std::logic_error(string("\n[") +
                               __FUNCTION__ +
                               string("] !Error during executing Python code\n"));
    }

    return {};
}

bool PyHandler::doGetExpandedLines(string& line)
{
    std::unique_lock<std::mutex>(pyContextLock);
    bool ret = false;
    
    try{
        if(PyObject_HasAttrString(mObj->ptr(), on_display_line)){
            vector<string> ss;
            boost::python::object o = mObj->attr(on_display_line)(line.c_str());
            char *data = extract<char*>(o);
            line = data;
            ret = true;
        }
    } catch (error_already_set& e) {
        PyErr_PrintEx(0);
        throw std::logic_error(string("\n[") +
                               __FUNCTION__ +
                               string("] !Error during executing Python code\n"));
    }

    return ret;
}

void PyHandler::updateAppViews()
{
    mPythonPlugin->updateAppViews();
}

