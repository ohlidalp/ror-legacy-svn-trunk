!define File "installerfiles\RoR.exe"

OutFile "ExtractVersionInfo.exe"
SilentInstall silent
RequestExecutionLevel user

Section

GetDLLVersion "${File}" $R0 $R1
IntOp $R2 $R0 >> 16
IntOp $R2 $R2 & 0x0000FFFF ; $R2 now contains major version
IntOp $R3 $R0 & 0x0000FFFF ; $R3 now contains minor version
IntOp $R4 $R1 >> 16
IntOp $R4 $R4 & 0x0000FFFF ; $R4 now contains release
IntOp $R5 $R1 & 0x0000FFFF ; $R5 now contains build
StrCpy $R1 "$R2.$R3.$R4.$R5"

 ## Write it to a !define for use in main script
 FileOpen $R0 "$EXEDIR\extracted-versioninfo.txt" w
  FileWrite $R0 '!define PRODUCT_VERSION_MAJOR "$R2"$\r$\n'
  FileWrite $R0 '!define PRODUCT_VERSION_MINOR "$R3"$\r$\n'
  FileWrite $R0 '!define PRODUCT_VERSION_PATCH "$R4"$\r$\n'
  FileWrite $R0 '!define PRODUCT_VERSION_REV   "$R5"$\r$\n'
  FileWrite $R0 '!define PRODUCT_VERSION_FULL  "$R1"$\r$\n'
 FileClose $R0

SectionEnd