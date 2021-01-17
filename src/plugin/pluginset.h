#ifndef PLUGINSET_H
#define PLUGINSET_H

#include "persistable.h"
#include <map>
#include <string>

using namespace std;

class PluginSet : public Persistable
{
public:
    PluginSet();

    // Persistable interface
public:
    void saveToStorage(QSettings &settings) const override;
    void retrieveFromStorage(QSettings &settings) override;
    bool isPluginSystemEnabled()const { return isPluginSystemEnabled_; }
    void setPluginSystemEnabled(bool v) { isPluginSystemEnabled_ = v; }
    map<string, bool> getPlugins() const { return plugins_; }
    void setPlugins(const map<string, bool>& plugins) { plugins_ = plugins; }
private:
    bool isPluginSystemEnabled_ = false;
    map<string, bool> plugins_;
};

#endif // PLUGINSET_H