#ifndef PLUGINSET_H
#define PLUGINSET_H

#include "persistable.h"

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
private:
    bool isPluginSystemEnabled_ = false;
};

#endif // PLUGINSET_H