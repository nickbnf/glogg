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
};

#endif // PLUGINSET_H