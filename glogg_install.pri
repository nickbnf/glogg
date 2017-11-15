icon16.path  = $$PREFIX/share/icons/hicolor/16x16/apps
icon16.files = images/hicolor/16x16/glogg.png

icon32.path  = $$PREFIX/share/icons/hicolor/32x32/apps
icon32.files = images/hicolor/32x32/glogg.png

icon_svg.path  = $$PREFIX/share/icons/hicolor/scalable/apps
icon_svg.files = images/hicolor/scalable/glogg.svg

doc.path  = $$PREFIX/share/doc/glogg
doc.files += README COPYING

desktop.path = $$PREFIX/share/applications
desktop.files = glogg.desktop

target.path = $$PREFIX/bin
INSTALLS = target icon16 icon32 icon_svg doc desktop
