; flow5 installer, built by packaging/windows/package.sh:
;   makensis -DVERSION=... -DSRCDIR=... -DOUTFILE=... -DICON=... flow5.nsi

Unicode true
!include "MUI2.nsh"

!define APPNAME "flow5"
!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

Name "${APPNAME} ${VERSION}"
OutFile "${OUTFILE}"
InstallDir "$PROGRAMFILES64\${APPNAME}"
InstallDirRegKey HKLM "Software\${APPNAME}" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma

!define MUI_ICON "${ICON}"
!define MUI_UNICON "${ICON}"
!define MUI_FINISHPAGE_RUN "$INSTDIR\flow5.exe"

!insertmacro MUI_PAGE_LICENSE "${SRCDIR}\LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

; The uninstaller removes $INSTDIR recursively, so never let it be a shared folder
Function .onVerifyInstDir
    StrCpy $0 "$INSTDIR" "" -6
    StrCmp $0 "\flow5" +2
        Abort
FunctionEnd

Section "Install"
    SetShellVarContext all
    SetOutPath "$INSTDIR"
    File /r "${SRCDIR}\*.*"

    WriteUninstaller "$INSTDIR\uninstall.exe"
    CreateShortcut "$SMPROGRAMS\${APPNAME}.lnk" "$INSTDIR\flow5.exe"
    CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\flow5.exe"

    WriteRegStr HKLM "Software\${APPNAME}" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "${UNINSTKEY}" "DisplayName" "${APPNAME}"
    WriteRegStr HKLM "${UNINSTKEY}" "DisplayVersion" "${VERSION}"
    WriteRegStr HKLM "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\flow5.exe"
    WriteRegStr HKLM "${UNINSTKEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKLM "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
    WriteRegDWORD HKLM "${UNINSTKEY}" "NoModify" 1
    WriteRegDWORD HKLM "${UNINSTKEY}" "NoRepair" 1
SectionEnd

Section "Uninstall"
    SetShellVarContext all
    Delete "$SMPROGRAMS\${APPNAME}.lnk"
    Delete "$DESKTOP\${APPNAME}.lnk"
    RMDir /r "$INSTDIR"
    DeleteRegKey HKLM "${UNINSTKEY}"
    DeleteRegKey HKLM "Software\${APPNAME}"
SectionEnd
