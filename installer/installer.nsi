Unicode True

; General
!define APP_NAME        "Phantom Pad"
!define APP_EXE         "phantompad.exe"
!define SERVICE_NAME    "PhantomPad"
!define SERVICE_DISPLAY "Phantom Pad Controller Server"
!define SERVICE_DESC    "Emulates a game controller using an Android device"
!define VIGEM_EXE       "ViGEmBus_1.22.0_x64_x86_arm64.exe"
!define VIGEM_UPGCODE   "{67175F6C-AA18-43A7-AE60-2FC3FD10BF79}"
!define REGKEY          "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"

; Version is set in CI pipeline
!ifndef APP_VERSION
    !define APP_VERSION "dev"
!endif

Name    "${APP_NAME} ${APP_VERSION}"
OutFile "phantompad-${APP_VERSION}-setup.exe"

InstallDir "$PROGRAMFILES64\${APP_NAME}"
InstallDirRegKey HKLM "${REGKEY}" "InstallLocation"

RequestExecutionLevel admin
SetCompressor /SOLID lzma

; UI
!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!include "FileFunc.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON   "..\res\icon.ico"
!define MUI_UNICON "..\res\icon.ico"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; Installer
Function .onInit
    ; Win10+
    ${IfNot} ${AtLeastWin10}
        MessageBox MB_OK|MB_ICONSTOP "Phantom Pad requires Windows 10 or later."
        Abort
    ${EndIf}

    ; 64-bit only
    ${IfNot} ${RunningX64}
        MessageBox MB_OK|MB_ICONSTOP "Phantom Pad requires a 64-bit Windows installation."
        Abort
    ${EndIf}

    ; Prevent running twice
    System::Call 'kernel32::CreateMutex(p 0, i 1, t "PhantomPadSetup") p .r0 ?e'
    Pop $0
    ${If} $0 = 183 ; ERROR_ALREADY_EXISTS
        MessageBox MB_OK|MB_ICONSTOP "The installer is already running."
        Abort
    ${EndIf}
FunctionEnd

Section "Phantom Pad" SecMain
    SectionIn RO

    SimpleSC::StopService "${SERVICE_NAME}" 1 30
    Pop $0

    SetOutPath "$INSTDIR"

    File "${APP_EXE}"
    File "THIRDPARTY.txt"

    Call InstallViGEm

    SimpleSC::InstallService "${SERVICE_NAME}" "${SERVICE_DISPLAY}" 16 2 "$INSTDIR\${APP_EXE}" "" "" ""
    Pop $0
    ${If} $0 <> 0
        ; Service already exists — just update the binary path
        SimpleSC::SetServiceBinaryPath "${SERVICE_NAME}" "$INSTDIR\${APP_EXE}"
        Pop $0
    ${EndIf}

    SimpleSC::SetServiceDescription "${SERVICE_NAME}" "${SERVICE_DESC}"
    Pop $0
    SimpleSC::StartService "${SERVICE_NAME}" "" 30
    Pop $0

    WriteUninstaller "$INSTDIR\uninstall.exe"

    SetRegView 64
    WriteRegStr   HKLM "${REGKEY}" "DisplayName"     "${APP_NAME}"
    WriteRegStr   HKLM "${REGKEY}" "DisplayVersion"  "${APP_VERSION}"
    WriteRegStr   HKLM "${REGKEY}" "Publisher"       "prevter"
    WriteRegStr   HKLM "${REGKEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr   HKLM "${REGKEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKLM "${REGKEY}" "NoModify"        1
    WriteRegDWORD HKLM "${REGKEY}" "NoRepair"        1

    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    WriteRegDWORD HKLM "${REGKEY}" "EstimatedSize" $0
    SetRegView lastused
SectionEnd

Function ViGEmInstalled
    SetRegView 64
    StrCpy $0 0
    StrCpy $3 0
    loop:
        EnumRegKey $1 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall" $3
        StrCmp $1 "" notfound

        ReadRegStr $2 HKLM \
            "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$1" \
            "BundleUpgradeCode"
        ${If} $2 == "${VIGEM_UPGCODE}"
            StrCpy $0 1
            SetRegView lastused
            Return
        ${EndIf}

        IntOp $3 $3 + 1
        Goto loop
    notfound:
        StrCpy $0 0
        SetRegView lastused
FunctionEnd

Function InstallViGEm
    Call ViGEmInstalled
    ${If} $0 = 1
        Return
    ${EndIf}

    DetailPrint "Installing ViGEmBus driver..."
    File "/oname=$TEMP\${VIGEM_EXE}" "deps\${VIGEM_EXE}"

    ExecWait '"$TEMP\${VIGEM_EXE}" /exenoui /qn' $0
    Delete "$TEMP\${VIGEM_EXE}"

    ${If} $0 <> 0
        MessageBox MB_OK|MB_ICONEXCLAMATION \
            "ViGEmBus installation failed (code $0).$\nYou can install it manually from:$\nhttps://github.com/nefarius/ViGEmBus/releases/latest"
    ${EndIf}
FunctionEnd

Section "Uninstall"
    SimpleSC::StopService   "${SERVICE_NAME}" 1 30
    Pop $0
    SimpleSC::RemoveService "${SERVICE_NAME}"
    Pop $0

    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\THIRDPARTY.txt"
    Delete "$INSTDIR\uninstall.exe"
    RMDir  "$INSTDIR"

    SetRegView 64
    DeleteRegKey HKLM "${REGKEY}"
    SetRegView lastused

    MessageBox MB_YESNO|MB_ICONQUESTION \
        "Do you want to also uninstall the ViGEmBus driver?$\nOnly say yes if no other applications use it." \
        IDNO SkipViGEm

        Call un.UninstallViGEm

    SkipViGEm:
SectionEnd

Function un.UninstallViGEm
    SetRegView 64
    StrCpy $0 0
    loop:
        EnumRegKey $1 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall" $0
        StrCmp $1 "" done

        ReadRegStr $2 HKLM \
            "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$1" \
            "BundleUpgradeCode"
        ${If} $2 == "${VIGEM_UPGCODE}"
            ReadRegStr $3 HKLM \
                "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$1" \
                "QuietUninstallString"
            SetRegView lastused
            ExecWait '$3'
            Return
        ${EndIf}

        IntOp $0 $0 + 1
        Goto loop
    done:
        SetRegView lastused
FunctionEnd