!include "MUI2.nsh"

Name "TwinkieX for TMSX"
OutFile "Output/TwinkieXTMSX.exe"
InstallDir "$LOCALAPPDATA\TrackMania Sunrise\"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section ""
    SetOutPath $INSTDIR
    File "..\..\Builds\Win32\TMSX\TwinkieTMSX.dll"
    File "..\ReleaseDeps\TMSX\dinput8.dll"
    File "..\ReleaseDeps\TMSX\discord_game_sdk.dll"
    SetOutPath "$DOCUMENTS\TwinkieX\"
    File /r "..\ReleaseDeps\DocumentsFolder\Fonts"
SectionEnd