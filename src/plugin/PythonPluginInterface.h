#ifndef PYTHONPLUGININTERFACE_H
#define PYTHONPLUGININTERFACE_H

#include <string>
#include <map>
#include <functional>
#include "data/search_result.h"
#include <functional>

using namespace std;

class PyHandler;
class AbstractLogView;

class PythonPluginInterface
{
public:
    virtual ~PythonPluginInterface() = default;

    virtual void createInstances() = 0;
    virtual void onPopupMenu(AbstractLogView* alv) = 0;
    virtual void onCreateMenu(AbstractLogView *alv) = 0;
    virtual bool isOnSearcAvailable() = 0;
    virtual SearchResultArray doSearch(const string &fileName, const string &pattern, int initialLine) = 0;
    virtual void doGetExpandedLines(string &line) = 0;
    virtual map<string, bool> getConfig() const = 0;
    virtual void setPluginState(const string& typeName, bool state) = 0;
    virtual void registerUpdateViewsFunction(function<void()> updateViewsFun) = 0;
    virtual bool isEnabled() = 0;
    virtual void enable(bool set) = 0;
    virtual void updateAppViews() = 0;
    virtual void onCreateToolBars(function<void(string tooltip, string icon, string pluginName, function<void(string)> action)> createAction) = 0;
    virtual void onCreateToolBarItem(string pluginName) = 0;
};

#endif // PYTHONPLUGININTERFACE_H
