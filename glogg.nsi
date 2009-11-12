# NSIS script creating the Windows installer for glogg

# Is passed to the script using -DVERSION=$(git describe) on the command line
!ifndef VERSION
    !define VERSION 'anonymous-build'
!endif

# Headers
!include "MUI2.nsh"

# General
OutFile "glogg-${VERSION}-setup.exe"

XpStyle on

SetCompressor /SOLID lzma

; Registry key to keep track of the directory we are installed in
InstallDir "$PROGRAMFILES\glogg"
InstallDirRegKey HKLM Software\glogg ""

; glogg icon
; !define MUI_ICON glogg.ico

RequestExecutionLevel user

Name "glogg"
Caption "glogg ${VERSION} Setup"

# Pages
!define MUI_WELCOMEPAGE_TITLE "Welcome to the glogg ${VERSION} Setup Wizard"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of logcrawer, a log browser.\
$\r$\n$\r$\nglogg is blah.$\r$\n$\r$\n\
glogg is released under the GPL, see http://www.fsf.org/licensing/licenses/gpl.html$\r$\n\
Qt libraries are released under the GPL, see http://qt.nokia.com/products/licensing$\r$\n$\r$\n$_CLICK"
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
    File release\mingwm10.dll
    File COPYING
    File README.textile

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

Section "Qt4 Runtime libraries" qtlibs
    SetOutPath $INSTDIR
    File release\QtCore4.dll
    File release\QtGui4.dll
SectionEnd

# Descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${glogg} "The core files required to use glogg."
    !insertmacro MUI_DESCRIPTION_TEXT ${qtlibs} "Needed by glogg, you have to install these unless \
you already have the Qt4 development kit installed."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

# Uninstaller
Section "Uninstall"
    Delete "$INSTDIR\Uninstall.exe"

    Delete "$INSTDIR\glogg.exe"
    Delete "$INSTDIR\README.textile"
    Delete "$INSTDIR\COPYING"
    Delete "$INSTDIR\mingwm10.dll"
    Delete "$INSTDIR\QtCore4.dll"
    Delete "$INSTDIR\QtGui4.dll"
    RMDir "$INSTDIR"

    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\glogg"
    DeleteRegKey /ifempty HKCU "Software\glogg"

    ; Remove the shortcut, if any
    SetShellVarContext all
    Delete "$SMPROGRAMS\glogg.lnk"
SectionEnd
