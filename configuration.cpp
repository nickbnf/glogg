#include "configuration.h"

Configuration::Configuration()
{
}

Configuration& Config()
{
    static Configuration conf;
    return conf;
}

// Accessor functions
QFont Configuration::mainFont() const
{
    return m_mainFont;
}

void Configuration::setMainFont(QFont newFont)
{
    m_mainFont = newFont;
}

// Read/Write functions
void Configuration::read(QSettings& settings)
{
    QString family = settings.value("mainFont.family").toString();
    int size = settings.value("mainFont.size").toInt();
    m_mainFont = QFont(family, size);
}

void Configuration::write(QSettings& settings)
{
    settings.setValue("mainFont.family", m_mainFont.family());
    settings.setValue("mainFont.size", m_mainFont.pointSize());
}
