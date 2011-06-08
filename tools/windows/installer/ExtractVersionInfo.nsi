!define File "installerfiles/RoR.exe"

OutFile "ExtractVersionInfo.exe"
SilentInstall silent
RequestExecutionLevel user

Section

 ## Get file version
 GetDllVersion "${File}" $R0 $R1
  IntOp $R2 $R0 / 0x00010000
  IntOp $R3 $R0 & 0x0000FFFF
  IntOp $R4 $R1 / 0x00010000
  IntOp $R5 $R1 & 0x0000FFFF
 # StrCpy $R1 "$R2.$R3.$R4.$R5"

 ## Write it to a !define for use in main script
 FileOpen $R0 "$EXEDIR\extracted-versioninfo.txt" w
  FileWrite $R0 '!define PRODUCT_VERSION_MAJOR "$R2"$\r$\n'
  FileWrite $R0 '!define PRODUCT_VERSION_MINOR "$R3"$\r$\n'
  FileWrite $R0 '!define PRODUCT_VERSION_PATCH "$R4"$\r$\n'
  FileWrite $R0 '!define PRODUCT_VERSION_REV   "$R5"$\r$\n'
 FileClose $R0

SectionEnd