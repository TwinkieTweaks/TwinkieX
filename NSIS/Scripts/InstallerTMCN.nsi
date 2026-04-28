!include "MUI2.nsh"

Name "TwinkieX for TMCN"
OutFile "Output/TwinkieXTMCN.exe"
InstallDir "$LOCALAPPDATA\TrackmaniaChina\"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section ""
    SetOutPath $INSTDIR
    File "..\..\Builds\x64\TMCN\TwinkieTMCN.dll"
    File "..\ReleaseDeps\TMCN\dinput8.dll"
SectionEnd