!include "MUI2.nsh"

Name "TwinkieX for TMO"
OutFile "Output/TwinkieXTMO.exe"
InstallDir "$LOCALAPPDATA\TrackMania Original\"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section ""
    SetOutPath $INSTDIR
    File "..\..\Builds\Win32\TMO\TwinkieTMO.dll"
    File "..\..\Builds\Win32\TMO\dinput8.dll"
    File "..\ReleaseDeps\TMO\discord_game_sdk.dll"
    SetOutPath "$DOCUMENTS\TwinkieX\"
    File /r "..\ReleaseDeps\DocumentsFolder\Fonts"
SectionEnd