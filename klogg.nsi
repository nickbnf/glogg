# NSIS script creating the Windows installer for glogg

# Is passed to the script using -DVERSION=$(git describe) on the command line
!ifndef VERSION
    !define VERSION 'anonymous-build'
!endif

# Headers
!include "MUI2.nsh"
!include "FileAssociation.nsh"

# General
OutFile "klogg-${VERSION}-setup.exe"

XpStyle on

SetCompressor /SOLID lzma

; Registry key to keep track of the directory we are installed in
!ifdef ARCH32
  InstallDir "$PROGRAMFILES\klogg"
!else
  InstallDir "$PROGRAMFILES64\klogg"
!endif
InstallDirRegKey HKLM Software\klogg ""

; glogg icon
; !define MUI_ICON glogg.ico

RequestExecutionLevel admin

Name "klogg"
Caption "klogg ${VERSION} Setup"

# Pages
!define MUI_WELCOMEPAGE_TITLE "Welcome to the klogg ${VERSION} Setup Wizard"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of klogg\
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
Section "klogg" klogg
    ; Prevent this section from being unselected
    SectionIn RO

    SetOutPath $INSTDIR
    File release\klogg.exe

    File COPYING
    File README.md

    File /nonfatal release\libstdc++-6.dll
    File /nonfatal release\libgcc_s_seh-1.dll
    File /nonfatal release\libgcc_s_dw2-1.dll

    ; Create the 'sendto' link
    CreateShortCut "$SENDTO\klogg.lnk" "$INSTDIR\klogg,exe" "" "$INSTDIR\klogg.exe" 0

    ; Register as an otion (but not main handler) for some files (.txt, .Log, .cap)
    WriteRegStr HKCR "Applications\klogg.exe" "" ""
    WriteRegStr HKCR "Applications\klogg.exe\shell" "" "open"
    WriteRegStr HKCR "Applications\klogg.exe\shell\open" "FriendlyAppName" "klogg"
    WriteRegStr HKCR "Applications\klogg.exe\shell\open\command" "" '"$INSTDIR\klogg.exe" "%1"'
    WriteRegStr HKCR "*\OpenWithList\klogg.exe" "" ""
    WriteRegStr HKCR ".txt\OpenWithList\klogg.exe" "" ""
    WriteRegStr HKCR ".Log\OpenWithList\klogg.exe" "" ""
    WriteRegStr HKCR ".cap\OpenWithList\klogg.exe" "" ""

    ; Register uninstaller
    WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg"\
"UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg"\
"InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg" "DisplayName" "klogg"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg" "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg" "NoModify" "1"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg" "NoRepair" "1"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Qt5 Runtime libraries" qtlibs
    SetOutPath $INSTDIR
    File release\Qt5Core.dll
    File release\Qt5Gui.dll
    File release\Qt5Network.dll
    File release\Qt5Widgets.dll
    File release\Qt5Concurrent.dll
    SetOutPath $INSTDIR\platforms
    File release\platforms\qwindows.dll
    File release\platforms\qminimal.dll
SectionEnd

Section "Create Start menu shortcut" shortcut
    SetShellVarContext all
    CreateShortCut "$SMPROGRAMS\klogg.lnk" "$INSTDIR\klogg.exe" "" "$INSTDIR\klogg.exe" 0
SectionEnd

Section /o "Associate with .log files" associate
    ${registerExtension} "$INSTDIR\klogg.exe" ".log" "Log file"
SectionEnd

# Descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${klogg} "The core files required to use klogg."
    !insertmacro MUI_DESCRIPTION_TEXT ${qtlibs} "Needed by klogg, you have to install these unless \
you already have the Qt5 development kit installed."
    !insertmacro MUI_DESCRIPTION_TEXT ${shortcut} "Create a shortcut in the Start menu for klogg."
    !insertmacro MUI_DESCRIPTION_TEXT ${associate} "Make klogg the default viewer for .log files."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

# Uninstaller
Section "Uninstall"
    Delete "$INSTDIR\Uninstall.exe"

    Delete "$INSTDIR\klogg.exe"
    Delete "$INSTDIR\README.md"
    Delete "$INSTDIR\COPYING"
    Delete "$INSTDIR\libstdc++-6.dll"
    Delete "$INSTDIR\libgcc_s_seh-1.dll"
    Delete "$INSTDIR\libgcc_s_dw2-1.dll"
    Delete "$INSTDIR\Qt5Widgets.dll"
    Delete "$INSTDIR\Qt5Core.dll"
    Delete "$INSTDIR\Qt5Gui.dll"
    Delete "$INSTDIR\Qt5Network.dll"
    Delete "$INSTDIR\Qt5Concurrent.dll"
    Delete "$INSTDIR\platforms\qwindows.dll"
    Delete "$INSTDIR\platforms\qminimal.dll"
    RMDir "$INSTDIR"

    ; Remove settings in %appdata%
    Delete "$APPDATA\klogg\klogg.ini"
    RMDir "$APPDATA\klogg"

    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg"

    ; Remove the file associations
    ${unregisterExtension} ".log" "Log file"

    DeleteRegKey HKCR "*\OpenWithList\klogg.exe"
    DeleteRegKey HKCR ".txt\OpenWithList\klogg.exe"
    DeleteRegKey HKCR ".Log\OpenWithList\klogg.exe"
    DeleteRegKey HKCR ".cap\OpenWithList\klogg.exe"
    DeleteRegKey HKCR "Applications\klogg.exe\shell\open\command"
    DeleteRegKey HKCR "Applications\klogg.exe\shell\open"
    DeleteRegKey HKCR "Applications\klogg.exe\shell"
    DeleteRegKey HKCR "Applications\klogg.exe"

    ; Remove the shortcut, if any
    SetShellVarContext all
    Delete "$SMPROGRAMS\klogg.lnk"
SectionEnd
