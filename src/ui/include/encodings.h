//
// Created by omni on 16.12.2019.
//

#ifndef KLOGG_ENCODINGS_H
#define KLOGG_ENCODINGS_H

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // Q_OS_WIN


#include <QActionGroup>
#include <QMenu>
#include <QString>
#include <QTextCodec>

class EncodingMenu {
  public:
    static QMenu* generate( QActionGroup* actionGroup )
    {
        const std::map<QString, std::vector<int>> supportedEncodings
            = { { "Arabic", { 82, 2256 } },
                { "Baltic", { 7, 109, 2257 } },
                { "Celtic", { 110 } },
                { "Cyrillic", { 8, 2084, 2088, 2027, 2086, 2251 } },
                { "Central European", { 2250 } },
                { "Chinese", { 2026, 2025 } },
                { "Eastern European", { 5 } },
                { "Greek", { 10, 2253 } },
                { "Hebrew", { 85, 2255 } },
                { "Japanese", { 17, 18, 39 } },
                { "Korean", { -949, 38 } },
                { "Thai", { 2259 } },
                { "Turkish", { 6, 12, 2254 } },
                { "Western European", { 3, 4, 111, 2009, 2252 } },
                { "Vietnamese", { 2258 } } };

        std::vector<int> unicodeMibs = { 106, 1013, 1014, 1018, 1019 };

        QMenu* encodingsMenu = new QMenu( "Encoding" );

        auto autoEncoding = encodingsMenu->addAction( "Auto" );
        autoEncoding->setStatusTip( "Automatically detect the file's encoding" );
        autoEncoding->setCheckable( true );
        autoEncoding->setActionGroup( actionGroup );
        autoEncoding->setChecked( true );

        encodingsMenu->addSeparator();

        QTextCodec* systemCodec = nullptr;

#ifdef Q_OS_WIN
        auto systemCodePage = ::GetACP();
        systemCodec
            = QTextCodec::codecForName( QString( "CP%1" ).arg( systemCodePage ).toLatin1() );
#endif
        if ( systemCodec == nullptr ) {
            systemCodec = QTextCodec::codecForLocale();
        }

        auto systemEncodingName = QString( "System (%1)" ).arg( systemCodec->name().constData() );

        auto systemEncoding = encodingsMenu->addAction( systemEncodingName );
        systemEncoding->setCheckable( true );
        systemEncoding->setActionGroup( actionGroup );
        systemEncoding->setData( systemCodec->mibEnum() );

        encodingsMenu->addSeparator();

        const auto addItemsForMibs = [&](auto& menu, const auto& mibs)
        {
          for ( const auto mib : mibs ) {
              auto codec = QTextCodec::codecForMib( mib );
              if ( codec ) {
                  auto action = menu->addAction( QString::fromLatin1( codec->name() ) );
                  action->setData( mib );
                  action->setCheckable( true );
                  action->setActionGroup( actionGroup );
              }
          }
        };

        auto unicodeMenu = encodingsMenu->addMenu( "Unicode" );
        addItemsForMibs(unicodeMenu, unicodeMibs);

        encodingsMenu->addSeparator();

        for ( const auto& group : supportedEncodings ) {
            auto menu = encodingsMenu->addMenu( group.first );
            addItemsForMibs(menu, group.second);
        }

        return encodingsMenu;
    }
};

#endif // KLOGG_ENCODINGS_H
