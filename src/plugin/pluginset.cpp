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
    // Remove everything in case the array is shorter than the previous one
    //settings.remove("");
    settings.setValue( "pluginSystemEnabled", isPluginSystemEnabled_ );
//    settings.beginWriteArray( "filters" );
//    for (int i = 0; i < filterList.size(); ++i) {
//        settings.setArrayIndex(i);
//        filterList[i].saveToStorage( settings );
//    }
//    settings.endArray();
    settings.endGroup();
}

void PluginSet::retrieveFromStorage(QSettings &settings)
{
    settings.beginGroup( "PluginSet" );
    isPluginSystemEnabled_ = settings.value( "pluginSystemEnabled" ).toBool();
    settings.endGroup();
}