Name "Adaptive Multirate component v1.1.3"
OutFile "foo_input_amr.exe"

!packhdr "foo_input_amr.dat" "upx.exe --best --crp-ms=999999 foo_input_amr.dat"
SetCompress          auto
SetDatablockOptimize on
CRCCheck             on
AutoCloseWindow      false
ShowInstDetails      show
SetDateSave          on

OutFile "foo_input_amr.exe"

InstallDir "$PROGRAMFILES\foobar2000"
InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\foobar2000" "InstallDir"
DirText "Select the installation directory:"

Section "Installation"
  SetOutPath "$INSTDIR\components"
  File "..\Release\foo_input_amr.dll"
SectionEnd
