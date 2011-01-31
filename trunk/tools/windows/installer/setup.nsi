; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "Rigs of Rods"
!define PRODUCT_VERSION "0.38.5"
!define PRODUCT_PUBLISHER "Rigs of Rods Team"
!define PRODUCT_WEB_SITE "http://www.rigsofrods.com"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"


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

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "RoR-Setup-${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES\Rigs of Rods ${PRODUCT_VERSION}"


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
    ExecWait '"$R0\uninst.exe" /S _?=$R0'
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

	Banner::show /NOUNLOAD "cleaning cache directory"
    ; this will empty that directory (but not delete it)
    !insertmacro RemoveFilesAndSubDirs "$DOCUMENTS\Rigs of Rods\cache\"
	Banner::destroy
  
	; docs
	#File ..\..\doc\keysheet.pdf
	#File "..\..\doc\Things you can do in Rigs of Rods.pdf"
	; data
	File /r /x .svn installerfiles\*

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
	SetOutPath $INSTDIR
	WriteIniStr "$INSTDIR\forums.url" "InternetShortcut" "URL" "http://forum.rigsofrods.com"
	CreateDirectory "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}"
	CreateShortCut "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}\Rigs of Rods Forums.lnk" "$INSTDIR\forums.url"
	CreateShortCut "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}\Uninstall.lnk" "$INSTDIR\uninst.exe"
	CreateShortCut "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}\Rigs of Rods.lnk" "$INSTDIR\RoRConfig.exe"
	CreateShortCut "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}\Updater.lnk" "$INSTDIR\updater.exe"
	CreateShortCut "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}\Key Sheet.lnk" "$INSTDIR\keysheet.pdf"
	CreateShortCut "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}\Manual.lnk" "$INSTDIR\Things_you_can_do_in_Rigs_of_Rods.pdf"
SectionEnd

Section -Post

	Call InstallDirectX
	Call InstallVisualStudioRuntime

	WriteUninstaller "$INSTDIR\uninst.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"

	; add some registry info
	WriteRegStr HKCU "Software\RigsOfRods\" "directory" "$INSTDIR"
	WriteRegStr HKCU "Software\RigsOfRods\" "version" "${PRODUCT_VERSION}"

	WriteRegStr HKCR "rorserver" "" "URL:Rigs of Rods Server"
	WriteRegStr HKCR "rorserver" "Url Protocol" ""
	WriteRegStr HKCR "rorserver\DefaultIcon" "" ""
	WriteRegStr HKCR "rorserver\shell\open\command" "" '"$INSTDIR\RoR.exe" -wd="$INSTDIR" -join="%1"'
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
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
	DeleteRegKey HKCR "rorserver"
	DeleteRegKey HKCU "Software\RigsOfRods\"
	
	Delete "$INSTDIR\uninst.exe"
	RMDir  "/r" "$INSTDIR"

	RMDir  "/r" "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}"
	Delete "$STARTMENU.lnk"
	RMDir  "$INSTDIR"
	DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
	
	SetShellVarContext current
	MessageBox MB_YESNO|MB_ICONEXCLAMATION "Remove RoR user content in $DOCUMENTS/Rigs of Rods?" IDYES removedoc IDNO end
removedoc:
 	RMDir  "/r" "$DOCUMENTS\Rigs of Rods"
end:	
	SetAutoClose false
SectionEnd
