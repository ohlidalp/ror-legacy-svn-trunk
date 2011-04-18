!define PRODUCT_NAME "Rigs of Rods"

!define PRODUCT_VERSION_MAJOR "0"
!define PRODUCT_VERSION_MINOR "38"
!define PRODUCT_VERSION_PATCH "24"

!define PRODUCT_VERSION "${PRODUCT_VERSION_MAJOR}.${PRODUCT_VERSION_MINOR}.${PRODUCT_VERSION_PATCH}"

!define PRODUCT_FULLNAME "${PRODUCT_NAME} ${PRODUCT_VERSION}"
!define PRODUCT_PUBLISHER "Rigs of Rods Team"
!define PRODUCT_WEB_SITE "http://www.rigsofrods.com"

!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_FULLNAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

!include "FileFunc.nsh"

; we want to be admin to install this
RequestExecutionLevel admin

!include "FileAssociation.nsh"

; ################################################################
; appends \ to the path if missing
; example: !insertmacro GetCleanDir "c:\blabla"
; Pop $0 => "c:\blabla\"
!macro GetCleanDir INPUTDIR
  ; ATTENTION: USE ON YOUR OWN RISK!
  ; Please report bugs here: http://stefan.bertels.org/
  !define Index_GetCleanDir 'GetCleanDir_Line${__LINE__}'
  Push $R0
  Push $R1
  StrCpy $R0 "${INPUTDIR}"
  StrCmp $R0 "" ${Index_GetCleanDir}-finish
  StrCpy $R1 "$R0" "" -1
  StrCmp "$R1" "\" ${Index_GetCleanDir}-finish
  StrCpy $R0 "$R0\"
${Index_GetCleanDir}-finish:
  Pop $R1
  Exch $R0
  !undef Index_GetCleanDir
!macroend
 
  ; load language from command line /L=1033
 ; foo.exe /S /L=1033 /D=C:\Program Files\Foo
 ; or:
 ; foo.exe /S "/L=1033" /D="C:\Program Files\Foo"
 ; gpv "/L=" "1033"
 !macro GETPARAMETERVALUE SWITCH DEFAULT
   Push $0
   Push $1
   Push $2
   Push $3
   Push $4

 ;$CMDLINE='"My Setup\Setup.exe" /L=1033 /S'
   Push "$CMDLINE"
   Push '${SWITCH}"'
   !insertmacro StrStr
   Pop $0
   StrCmp "$0" "" gpv_notquoted
 ;$0='/L="1033" /S'
   StrLen $2 "$0"
   Strlen $1 "${SWITCH}"
   IntOp $1 $1 + 1
   StrCpy $0 "$0" $2 $1
 ;$0='1033" /S'
   Push "$0"
   Push '"'
   !insertmacro StrStr
   Pop $1
   StrLen $2 "$0"
   StrLen $3 "$1"
   IntOp $4 $2 - $3
   StrCpy $0 $0 $4 0
   Goto gpv_done

   gpv_notquoted:
   Push "$CMDLINE"
   Push "${SWITCH}"
   !insertmacro StrStr
   Pop $0
   StrCmp "$0" "" gpv_done
 ;$0='/L="1033" /S'
   StrLen $2 "$0"
   Strlen $1 "${SWITCH}"
   StrCpy $0 "$0" $2 $1
 ;$0=1033 /S'
   Push "$0"
   Push ' '
   !insertmacro StrStr
   Pop $1
   StrLen $2 "$0"
   StrLen $3 "$1"
   IntOp $4 $2 - $3
   StrCpy $0 $0 $4 0
   Goto gpv_done

   gpv_done:
   StrCmp "$0" "" 0 +2
   StrCpy $0 "${DEFAULT}"

   Pop $4
   Pop $3
   Pop $2
   Pop $1
   Exch $0
 !macroend

; And I had to modify StrStr a tiny bit.
; Possible upgrade switch the goto's to use ${__LINE__}

!macro STRSTR
  Exch $R1 ; st=haystack,old$R1, $R1=needle
  Exch    ; st=old$R1,haystack
  Exch $R2 ; st=old$R1,old$R2, $R2=haystack
  Push $R3
  Push $R4
  Push $R5
  StrLen $R3 $R1
  StrCpy $R4 0
  ; $R1=needle
  ; $R2=haystack
  ; $R3=len(needle)
  ; $R4=cnt
  ; $R5=tmp
 ;  loop;
    StrCpy $R5 $R2 $R3 $R4
    StrCmp $R5 $R1 +4
    StrCmp $R5 "" +3
    IntOp $R4 $R4 + 1
    Goto -4
 ;  done;
  StrCpy $R1 $R2 "" $R4
  Pop $R5
  Pop $R4
  Pop $R3
  Pop $R2
  Exch $R1
!macroend
; ################################################################
; similar to "RMDIR /r DIRECTORY", but does not remove DIRECTORY itself
; example: !insertmacro RemoveFilesAndSubDirs "$INSTDIR"
!macro RemoveFilesAndSubDirs DIRECTORY
  ; ATTENTION: USE ON YOUR OWN RISK!
  ; Please report bugs here: http://stefan.bertels.org/
  !define Index_RemoveFilesAndSubDirs 'RemoveFilesAndSubDirs_${__LINE__}'
 
  Push $R0
  Push $R1
  Push $R2
 
  !insertmacro GetCleanDir "${DIRECTORY}"
  Pop $R2
  FindFirst $R0 $R1 "$R2*.*"
${Index_RemoveFilesAndSubDirs}-loop:
  StrCmp $R1 "" ${Index_RemoveFilesAndSubDirs}-done
  StrCmp $R1 "." ${Index_RemoveFilesAndSubDirs}-next
  StrCmp $R1 ".." ${Index_RemoveFilesAndSubDirs}-next
  IfFileExists "$R2$R1\*.*" ${Index_RemoveFilesAndSubDirs}-directory
  ; file
  Delete "$R2$R1"
  goto ${Index_RemoveFilesAndSubDirs}-next
${Index_RemoveFilesAndSubDirs}-directory:
  ; directory
  RMDir /r "$R2$R1"
${Index_RemoveFilesAndSubDirs}-next:
  FindNext $R0 $R1
  Goto ${Index_RemoveFilesAndSubDirs}-loop
${Index_RemoveFilesAndSubDirs}-done:
  FindClose $R0
 
  Pop $R2
  Pop $R1
  Pop $R0
  !undef Index_RemoveFilesAndSubDirs
!macroend

Function WriteToFile
 Exch $0 ;file to write to
 Exch
 Exch $1 ;text to write
 
  FileOpen $0 $0 a #open file
   FileSeek $0 0 END #go to end
   FileWrite $0 $1 #write to file
  FileClose $0
 
 Pop $1
 Pop $0
FunctionEnd
 
!macro WriteToFile String File
 Push "${String}"
 Push "${File}"
  Call WriteToFile
!macroend
!define WriteToFile "!insertmacro WriteToFile"


SetCompressor /FINAL /SOLID lzma

BrandingText "Rigs of Rods"
InstProgressFlags smooth colored
XPStyle on
ShowInstDetails show
ShowUninstDetails show
SetDateSave on
#SetDatablockOptimize on
CRCCheck on
#SilentInstall normal

; MUI 1.67 compatible ------
!include "MUI.nsh"
!include "LogicLib.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install-blue.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall-blue.ico"

; Language Selection Dialog Settings
!define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "readme-installer.txt"

; Components page
;!insertmacro MUI_PAGE_COMPONENTS
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
;!define MUI_FINISHPAGE_RUN "$INSTDIR\rortoolkit.bat"
;!define MUI_FINISHPAGE_RUN_PARAMETERS ""
#!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\Example.file"

!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_RUN
;!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_RUN_TEXT "Check for updates"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchPostInstallation"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English" ;first language is the default language
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "SpanishInternational"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Danish"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "Norwegian"
!insertmacro MUI_LANGUAGE "NorwegianNynorsk"
!insertmacro MUI_LANGUAGE "Finnish"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Slovak"
!insertmacro MUI_LANGUAGE "Croatian"
!insertmacro MUI_LANGUAGE "Bulgarian"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Thai"
!insertmacro MUI_LANGUAGE "Romanian"
!insertmacro MUI_LANGUAGE "Latvian"
!insertmacro MUI_LANGUAGE "Macedonian"
!insertmacro MUI_LANGUAGE "Estonian"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Lithuanian"
!insertmacro MUI_LANGUAGE "Slovenian"
!insertmacro MUI_LANGUAGE "Serbian"
!insertmacro MUI_LANGUAGE "SerbianLatin"
!insertmacro MUI_LANGUAGE "Arabic"
!insertmacro MUI_LANGUAGE "Farsi"
!insertmacro MUI_LANGUAGE "Hebrew"
!insertmacro MUI_LANGUAGE "Indonesian"
!insertmacro MUI_LANGUAGE "Mongolian"
!insertmacro MUI_LANGUAGE "Luxembourgish"
!insertmacro MUI_LANGUAGE "Albanian"
!insertmacro MUI_LANGUAGE "Breton"
!insertmacro MUI_LANGUAGE "Belarusian"
!insertmacro MUI_LANGUAGE "Icelandic"
!insertmacro MUI_LANGUAGE "Malay"
!insertmacro MUI_LANGUAGE "Bosnian"
!insertmacro MUI_LANGUAGE "Kurdish"
!insertmacro MUI_LANGUAGE "Irish"
!insertmacro MUI_LANGUAGE "Uzbek"
!insertmacro MUI_LANGUAGE "Galician"
!insertmacro MUI_LANGUAGE "Afrikaans"
;!insertmacro MUI_LANGUAGE "Catalan"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS
; MUI end ------

Name "${PRODUCT_FULLNAME}"
OutFile "RoR-Setup-${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES\${PRODUCT_FULLNAME}"

!macro CreateInternetShortcut FILENAME URL ICONFILE ICONINDEX
WriteINIStr "${FILENAME}.url" "InternetShortcut" "URL" "${URL}"
WriteINIStr "${FILENAME}.url" "InternetShortcut" "IconFile" "${ICONFILE}"
WriteINIStr "${FILENAME}.url" "InternetShortcut" "IconIndex" "${ICONINDEX}"
!macroend

ShowInstDetails show
ShowUnInstDetails show

Function InstallDirectX
	Banner::show /NOUNLOAD "Installing latest DirectX"
	DetailPrint "Running DirectX Setup ..."
    ExecWait '"$INSTDIR\dxwebsetup.exe"' $0
	DetailPrint "dxwebsetup returned $0"
	Banner::destroy
FunctionEnd

Function InstallVisualStudioRuntime
	Banner::show /NOUNLOAD "Installing Visual Studio Runtime"
	DetailPrint "Installing Visual Studio Runtime ..."
	ExecWait '"$INSTDIR\vcredist_x86.exe" /q:a /c:"VCREDI~1.EXE /q:a /c:""msiexec /i vcredist.msi /qb!"" "' $0
	DetailPrint "VCCRT setup returned $0"
	Banner::destroy
FunctionEnd

Function UninstallOld
    ReadRegStr $R0 HKCU "Software\RigsOfRods\" "directory"
    StrCmp $R0 "" done
    ReadRegStr $R1 HKCU "Software\RigsOfRods\" "version"

    MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
    "${PRODUCT_NAME} version $R1 is already installed. $\n$\nClick `OK` to remove the \
    previous version or `Cancel` to cancel this upgrade." \
    IDOK uninst
    Abort
 
uninst:
    ClearErrors
	Banner::show /NOUNLOAD "uninstalling old version"
    ExecWait '"$R0\uninst.exe" /UPGRADE=1 /S _?=$R0'
    Delete "$R0\uninst.exe"
	Banner::destroy
done:    
FunctionEnd

; SPLASH
; if you dont want the splash comment out the following function:
Function .onInit
	InitPluginsDir
	File /oname=$PLUGINSDIR\splash.bmp "splash.bmp"
	advsplash::show 1000 1300 600 -1 $PLUGINSDIR\splash
	Pop $0
	Delete $PLUGINSDIR\splash
	!insertmacro MUI_LANGDLL_DISPLAY
    Call UninstallOld
FunctionEnd

Section "!RoR" SEC02
	SetOutPath "$INSTDIR"
	SetOverwrite try

	; data
	File /r /x .svn installerfiles\*

	# user path
	SetShellVarContext current
	SetOutPath "$DOCUMENTS\Rigs of Rods\"
	File /r /x .svn installerskeleton\*

	; clean cache directory
	Banner::show /NOUNLOAD "cleaning cache directory"
    ; this will empty that directory (but not delete it)
    !insertmacro RemoveFilesAndSubDirs "$DOCUMENTS\Rigs of Rods\cache\"
	Banner::destroy
	
	# add version text file
	${WriteToFile} "${PRODUCT_VERSION}" "$INSTDIR\version.txt"

SectionEnd

;Section "Modding Tools" SEC03
;    SetOutPath "$INSTDIR"
;    SetOverwrite try
;	File /r /x .svn ..\..\tools\modtool
;SectionEnd

Function "LaunchPostInstallation"
	Exec  "$INSTDIR\update.exe"
FunctionEnd

Section -AdditionalIcons
	SetShellVarContext current
	SetOutPath $INSTDIR
	
	CreateDirectory "$SMPROGRAMS\${PRODUCT_FULLNAME}"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_FULLNAME}\Uninstall.lnk" "$INSTDIR\uninst.exe"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_FULLNAME}\Rigs of Rods.lnk" "$INSTDIR\RoRConfig.exe"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_FULLNAME}\Updater.lnk" "$INSTDIR\updater.exe"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_FULLNAME}\Key Sheet.lnk" "$INSTDIR\keysheet.pdf"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_FULLNAME}\Manual.lnk" "$INSTDIR\Things_you_can_do_in_Rigs_of_Rods.pdf"

	!insertmacro CreateInternetShortcut "$SMPROGRAMS\${PRODUCT_FULLNAME}\Forums" "http://www.rigsofrods.com/forum.php" "$InstDir\cog_go.ico" "0"
	!insertmacro CreateInternetShortcut "$SMPROGRAMS\${PRODUCT_FULLNAME}\Beginners Guide" "http://www.rigsofrods.com/wiki/pages/Beginner%27s_Guide" "$InstDir\forums.ico" "0"
	
SectionEnd

Section -Post

	Call InstallDirectX
	Call InstallVisualStudioRuntime
	
	${registerExtension} "$INSTDIR\RoRMeshViewer.exe" ".mesh" "Ogre Mesh"

	WriteUninstaller "$INSTDIR\uninst.exe"
	; add software to windows uninstaller
	
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\RoR.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_FULLNAME}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
	
	
	${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
	IntFmt $0 "0x%08X" $0
	WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"

	WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" "1"
	WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" "1"
	WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "WindowsInstaller" "0"
	WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "VersionMajor" "${PRODUCT_VERSION_MAJOR}"
	WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "VersionMinor" "${PRODUCT_VERSION_MINOR}"
	
	 
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$\"$INSTDIR\uninst.exe$\""
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "QuietUninstallString" "$\"$INSTDIR\uninst.exe$\" /S"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "HelpLink" "http://www.rigsofrods.com/forums/95-Support"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLUpdateInfo" "http://www.rigsofrods.com/wiki/pages/Changelog"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Contact" "support@rigsofrods.com"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Comments" "Soft Body Physics Sandbox Game"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Readme" ""

	; add some registry info
	WriteRegStr HKCU "Software\RigsOfRods\" "directory" "$INSTDIR"
	WriteRegStr HKCU "Software\RigsOfRods\" "version" "${PRODUCT_VERSION}"

	WriteRegStr HKCR "rorserver" "" "URL:Rigs of Rods Server"
	WriteRegStr HKCR "rorserver" "Url Protocol" ""
	WriteRegStr HKCR "rorserver\DefaultIcon" "" ""
	WriteRegStr HKCR "rorserver\shell\open\command" "" '"$INSTDIR\RoR.exe" -wd="$INSTDIR" -join="%1"'
	SetRebootFlag true
SectionEnd


Function un.onUninstSuccess
	HideWindow
	MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was uninstalled successfully." /SD IDOK
FunctionEnd

Function un.onInit
    !insertmacro MUI_UNGETLANGUAGE
    MessageBox MB_ICONQUESTION|MB_YESNO "Do you want to uninstall $(^Name)?" /SD IDYES IDNO no IDYES yes
no:
    Abort
yes:
FunctionEnd

Section Uninstall
	; remove installed files
	;Delete "$INSTDIR\uninst.exe"
	RMDir /r "$INSTDIR\*.*"   
	RMDir "$INSTDIR"

	SetShellVarContext current
	Delete "$SMPROGRAMS\${PRODUCT_FULLNAME}\*.*"
	RmDir  "$SMPROGRAMS\${PRODUCT_FULLNAME}"
	
	; remove registry entries
	DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
	DeleteRegKey HKCU "Software\RigsOfRods\"
	DeleteRegKey HKCR "rorserver"
	
	${unregisterExtension} ".mesh" "Ogre Mesh"

	; if upgrading, do not remove user content
	!insertmacro GetParameterValue "/UPGRADE=" "0"
	Pop $0
	StrCmp $0 "1" end

	SetShellVarContext current
	MessageBox MB_YESNO|MB_ICONEXCLAMATION "Remove RoR user content in $DOCUMENTS\Rigs of Rods?" IDYES removedoc IDNO end
removedoc:
	MessageBox MB_YESNO|MB_ICONEXCLAMATION "Are you really sure to remove all user content in $DOCUMENTS\Rigs of Rods?" IDYES removedoc2 IDNO end
removedoc2:
	RMDir  "/r" "$DOCUMENTS\Rigs of Rods"
end:
	SetAutoClose false
SectionEnd
