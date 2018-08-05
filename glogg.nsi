# NSIS script creating the Windows installer for glogg

# Is passed to the script using -DVERSION=$(git describe) on the command line
!ifndef VERSION
    !define VERSION 'anonymous-build'
!endif

# Headers
!include "MUI2.nsh"
!include "FileAssociation.nsh"

# General
OutFile "glogg-${VERSION}-setup.exe"

XpStyle on

SetCompressor /SOLID lzma

; Registry key to keep track of the directory we are installed in
!ifdef ARCH32
  InstallDir "$PROGRAMFILES\glogg"
!else
  InstallDir "$PROGRAMFILES64\glogg"
!endif
InstallDirRegKey HKLM Software\glogg ""

; glogg icon
; !define MUI_ICON glogg.ico

RequestExecutionLevel admin

Name "glogg"
Caption "glogg ${VERSION} Setup"

# Pages
!define MUI_WELCOMEPAGE_TITLE "Welcome to the glogg ${VERSION} Setup Wizard"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of glogg\
, a fast, advanced log explorer.$\r$\n$\r$\n\
glogg and the Qt libraries are released under the GPL, see \
the COPYING file.$\r$\n$\r$\n$_CLICK"
; MUI_FINISHPAGE_LINK_LOCATION "http://nsis.sf.net/"

!insertmacro MUI_PAGE_WELCOME
;!insertmacro MUI_PAGE_LICENSE "COPYING"
# !ifdef VER_MAJOR & VER_MINOR & VER_REVISION & VER_BUILD...
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

# Languages
!insertmacro MUI_LANGUAGE "English"

# Installer sections
Section "glogg" glogg
    ; Prevent this section from being unselected
    SectionIn RO

    SetOutPath $INSTDIR
    File release\glogg.exe
    File COPYING
    File README.md

    ; Create the 'sendto' link
    CreateShortCut "$SENDTO\glogg.lnk" "$INSTDIR\glogg,exe" "" "$INSTDIR\glogg.exe" 0

    ; Register as an otion (but not main handler) for some files (.txt, .Log, .cap)
    WriteRegStr HKCR "Applications\glogg.exe" "" ""
    WriteRegStr HKCR "Applications\glogg.exe\shell" "" "open"
    WriteRegStr HKCR "Applications\glogg.exe\shell\open" "FriendlyAppName" "glogg"
    WriteRegStr HKCR "Applications\glogg.exe\shell\open\command" "" '"$INSTDIR\glogg.exe" "%1"'
    WriteRegStr HKCR "*\OpenWithList\glogg.exe" "" ""
    WriteRegStr HKCR ".txt\OpenWithList\glogg.exe" "" ""
    WriteRegStr HKCR ".Log\OpenWithList\glogg.exe" "" ""
    WriteRegStr HKCR ".cap\OpenWithList\glogg.exe" "" ""

    ; Register uninstaller
    WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\glogg"\
"UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\glogg"\
"InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\glogg" "DisplayName" "glogg"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\glogg" "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\glogg" "NoModify" "1"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\glogg" "NoRepair" "1"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Qt5 Runtime libraries" qtlibs
    SetOutPath $INSTDIR
    File release\Qt5Core.dll
    File release\Qt5Gui.dll
    File release\Qt5Network.dll
    File release\Qt5Widgets.dll
    File release\libwinpthread-1.dll
    SetOutPath $INSTDIR\platforms
    File release\qwindows.dll
SectionEnd

Section "Create Start menu shortcut" shortcut
    SetShellVarContext all
    CreateShortCut "$SMPROGRAMS\glogg.lnk" "$INSTDIR\glogg.exe" "" "$INSTDIR\glogg.exe" 0
SectionEnd

Section /o "Associate with .log files" associate
    ${registerExtension} "$INSTDIR\glogg.exe" ".log" "Log file"
SectionEnd

# Descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${glogg} "The core files required to use glogg."
    !insertmacro MUI_DESCRIPTION_TEXT ${qtlibs} "Needed by glogg, you have to install these unless \
you already have the Qt5 development kit installed."
    !insertmacro MUI_DESCRIPTION_TEXT ${shortcut} "Create a shortcut in the Start menu for glogg."
    !insertmacro MUI_DESCRIPTION_TEXT ${associate} "Make glogg the default viewer for .log files."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

# Uninstaller
Section "Uninstall"
    Delete "$INSTDIR\Uninstall.exe"

    Delete "$INSTDIR\glogg.exe"
    Delete "$INSTDIR\README"
    Delete "$INSTDIR\README.md"
    Delete "$INSTDIR\COPYING"
    Delete "$INSTDIR\mingwm10.dll"
    Delete "$INSTDIR\libgcc_s_dw2-1.dll"
    Delete "$INSTDIR\QtCore4.dll"
    Delete "$INSTDIR\QtGui4.dll"
    Delete "$INSTDIR\QtNetwork4.dll"
    Delete "$INSTDIR\Qt5Core.dll"
    Delete "$INSTDIR\Qt5Gui.dll"
    Delete "$INSTDIR\Qt5Network.dll"
    Delete "$INSTDIR\Qt5Widgets.dll"
    Delete "$INSTDIR\libwinpthread-1.dll"
    Delete "$INSTDIR\platforms\qwindows.dll"
    RMDir "$INSTDIR"

    ; Remove settings in %appdata%
    Delete "$APPDATA\glogg\glogg.ini"
    RMDir "$APPDATA\glogg"

    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\glogg"

    ; Remove settings in the registry (from glogg < 0.9)
    DeleteRegKey HKCU "Software\glogg"

    ; Remove the file associations
    ${unregisterExtension} ".log" "Log file"

    DeleteRegKey HKCR "*\OpenWithList\glogg.exe"
    DeleteRegKey HKCR ".txt\OpenWithList\glogg.exe"
    DeleteRegKey HKCR ".Log\OpenWithList\glogg.exe"
    DeleteRegKey HKCR ".cap\OpenWithList\glogg.exe"
    DeleteRegKey HKCR "Applications\glogg.exe\shell\open\command"
    DeleteRegKey HKCR "Applications\glogg.exe\shell\open"
    DeleteRegKey HKCR "Applications\glogg.exe\shell"
    DeleteRegKey HKCR "Applications\glogg.exe"

    ; Remove the shortcut, if any
    SetShellVarContext all
    Delete "$SMPROGRAMS\glogg.lnk"
SectionEnd
