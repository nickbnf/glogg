# NSIS script creating the Windows installer for logcrawler

# Is passed to the script using -DVERSION=$(git describe) on the command line
!ifndef VERSION
    !define VERSION 'anonymous-build'
!endif

# Headers
!include "MUI2.nsh"

# General
OutFile "logcrawler-${VERSION}-setup.exe"

SetCompressor /SOLID lzma

InstallDir "$PROGRAMFILES\logcrawler"
InstallDirRegKey HKLM Software\logcrawler ""

RequestExecutionLevel user

Name "logcrawler"
Caption "logcrawler ${VERSION} Setup"

# Pages
!define MUI_WELCOMEPAGE_TITLE "Welcome to the logcrawler ${VERSION} Setup Wizard"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of logcrawer, a log browser.\
$\r$\n$\r$\nlogcrawler is blah.$\r$\n$\r$\n$_CLICK"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "COPYING"
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
Section "LogCrawler" logcrawler
    SetOutPath $INSTDIR
    File release\logcrawler.exe
    File release\mingwm10.dll
    File COPYING
    File README.textile

    ; Register uninstaller
    WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\logcrawler"\
"UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\logcrawler"\
"InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\logcrawler" "DisplayName" "LogCrawler"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\logcrawler" "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\logcrawler" "NoModify" "1"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\logcrawler" "NoRepair" "1"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "QT4 Runtime libraries" qtlibs
    SetOutPath $INSTDIR
    File release\QtCore4.dll
    File release\QtGui4.dll
SectionEnd

# Descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${logcrawler} "The core files required to use LogCrawler."
    !insertmacro MUI_DESCRIPTION_TEXT ${qtlibs} "Needed by LogCrawler, you have to install these unless \
you already have the QT4 development kit installed."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

# Uninstaller
Section "Uninstall"
    Delete "$INSTDIR\Uninstall.exe"

    Delete "$INSTDIR\logcrawler.exe"
    Delete "$INSTDIR\README.textile"
    Delete "$INSTDIR\COPYING"
    Delete "$INSTDIR\mingwm10.dll"
    Delete "$INSTDIR\QtCore4.dll"
    Delete "$INSTDIR\QtGui4.dll"
    RMDir "$INSTDIR"

    DeleteRegKey /ifempty HKCU "Software\logcrawler"
SectionEnd
