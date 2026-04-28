!include "MUI2.nsh"

Name "TwinkieX for ESWC"
OutFile "Output/TwinkieXESWC.exe"
InstallDir "$LOCALAPPDATA\Nadeo\Trackmania Nations ESWC\"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section ""
    SetOutPath $INSTDIR
    File "..\..\Builds\Win32\ESWC\TwinkieESWC.dll"
    File "..\ReleaseDeps\ESWC\dinput8.dll"
SectionEnd