#include "pluginset.h"
#include <QSettings>
#include <QDataStream>

#include "log.h"

PluginSet::PluginSet()
{

}


void PluginSet::saveToStorage(QSettings &settings) const
{
    LOG(logDEBUG) << "PluginSet::saveToStorage";

    settings.beginGroup( "PluginSet" );
    settings.remove("");
    settings.setValue( "pluginSystemEnabled", isPluginSystemEnabled_ );

    settings.beginWriteArray( "plugin" );
    // Remove everything in case the array is shorter than the previous one
    settings.remove("");

    int i = 0;
    for (const auto& item: plugins_) {
        settings.setArrayIndex(i++);
        settings.setValue("name", item.first.c_str());
        settings.setValue("enabled", item.second);
    }
    settings.endArray();
    settings.endGroup();
}

void PluginSet::retrieveFromStorage(QSettings &settings)
{
    settings.beginGroup( "PluginSet" );
    isPluginSystemEnabled_ = settings.value( "pluginSystemEnabled" ).toBool();

    int size = settings.beginReadArray( "plugin" );

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        plugins_[settings.value("name").toString().toStdString()] = settings.value("enabled").toBool();
    }
    settings.endArray();
    settings.endGroup();
}
