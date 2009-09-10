#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QFont>
#include <QSettings>

/// Configuration class created as a singleton
class Configuration {
    public:
        QFont mainFont() const;
        void setMainFont(QFont newFont);

        void read(QSettings& settings);
        void write(QSettings& settings);

    private:
        Configuration();
        Configuration(const Configuration&);

        // Configuration settings
        QFont m_mainFont;

        // allow this function to create one instance
        friend Configuration& Config();
};

Configuration& Config();

#endif
