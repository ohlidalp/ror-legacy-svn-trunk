; required NSIS plugins:
;   Inetc   - http://nsis.sourceforge.net/Inetc_plug-in
;   ZipDLL  - http://nsis.sourceforge.net/ZipDLL_plug-in

SetCompress auto
SetCompressor /FINAL /SOLID lzma

!define PRODUCT_NAME "Rigs of Rods"

!define PRODUCT_VERSION_MAJOR "0"
!define PRODUCT_VERSION_MINOR "38"
!define PRODUCT_VERSION_PATCH "28"

!define PRODUCT_VERSION "${PRODUCT_VERSION_MAJOR}.${PRODUCT_VERSION_MINOR}.${PRODUCT_VERSION_PATCH}"
!define PRODUCT_VERSION_SHORT "${PRODUCT_VERSION_MAJOR}.${PRODUCT_VERSION_MINOR}"

!define PRODUCT_FULLNAME "${PRODUCT_NAME} ${PRODUCT_VERSION}"
!define PRODUCT_PUBLISHER "Rigs of Rods Team"
!define PRODUCT_WEB_SITE "http://www.rigsofrods.com"

!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_FULLNAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

!define ROR_REGISTRY_ENTRY "Software\${PRODUCT_FULLNAME}"
!include "FileFunc.nsh"

Name "${PRODUCT_FULLNAME}"
!define OUTFILE "RoR-Setup-${PRODUCT_VERSION}.exe"
OutFile "${OUTFILE}"
InstallDir "$PROGRAMFILES\${PRODUCT_FULLNAME}"

; we want to be admin to install this
RequestExecutionLevel admin

; include macros we are going to use
!include "FileAssociation.nsh"
!include "utils.nsh"

Var StartMenuFolder

BrandingText "Rigs of Rods"
InstProgressFlags smooth colored
XPStyle on
ShowInstDetails show
ShowUninstDetails show
SetDateSave on
SetDatablockOptimize on
CRCCheck force
#SilentInstall normal
InstallDirRegKey HKLM "${ROR_REGISTRY_ENTRY}" "InstallLocation"
SetOverwrite try

InstType "full"
InstType "minimal"


; add file properties
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "The Rigs of Rods softbody simulation"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "www.rigsofrods.com"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" "www.rigsofrods.com"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "www.rigsofrods.com"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${PRODUCT_FULLNAME} Installer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${PRODUCT_VERSION}"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "InternalName" "RoR"
VIAddVersionKey /LANG=${LANG_ENGLISH} "InternalName" "RoR"
VIAddVersionKey /LANG=${LANG_ENGLISH} "OriginalFilename" "${OUTFILE}"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "PrivateBuild" "${PRODUCT_VERSION}"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "SpecialBuild" "${PRODUCT_VERSION}"
VIProductVersion "${PRODUCT_VERSION}.0" ; hacky add .0 to stay compatible with windows ...

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
!insertmacro MUI_PAGE_COMPONENTS
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; start menu
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_RUN
;!define MUI_FINISHPAGE_RUN_PARAMETERS ""
;!define MUI_FINISHPAGE_RUN_NOTCHECKED
;!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\Example.file"

;!define MUI_FINISHPAGE_RUN_TEXT "Check for updates"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchPostInstallation"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_DIRECTORY
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

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
	
	; no more splash :-/
	;File /oname=$PLUGINSDIR\splash.bmp "splash.bmp"
	;advsplash::show 1000 1300 600 -1 $PLUGINSDIR\splash
	;Pop $0
	;Delete $PLUGINSDIR\splash
	
	!insertmacro MUI_LANGDLL_DISPLAY
    #Call UninstallOld
FunctionEnd

Section "!Rigs of Rods Base" RoRBaseGame
	; in full and minimal
	SectionIn 1 2 RO
	SetOutPath "$INSTDIR"

	; data
	File /r /x .svn installerfiles\*

	# user path
	SetShellVarContext current
	SetOutPath "$DOCUMENTS\Rigs of Rods ${PRODUCT_VERSION_SHORT}\"
	# create some empty directories
	CreateDirectory "$DOCUMENTS\Rigs of Rods ${PRODUCT_VERSION_SHORT}\cache\"
	CreateDirectory "$DOCUMENTS\Rigs of Rods ${PRODUCT_VERSION_SHORT}\logs\"
	CreateDirectory "$DOCUMENTS\Rigs of Rods ${PRODUCT_VERSION_SHORT}\terrains\"
	CreateDirectory "$DOCUMENTS\Rigs of Rods ${PRODUCT_VERSION_SHORT}\vehicles\"
	# no file overwriting for the user folder, important so the user can keep his configuration
	SetOverwrite off
	File /r /x .svn installerskeleton\*
	# install config files without overwrite on
	SetOverwrite try
	# overwrite some configuration files that are required
	SetOutPath "$DOCUMENTS\Rigs of Rods ${PRODUCT_VERSION_SHORT}\config\"
	File installerskeleton\config\categories.cfg
	File installerskeleton\config\editor.cfg
	File installerskeleton\config\ground_models.cfg
	File installerskeleton\config\inertia_models.cfg
	File installerskeleton\config\skidmarks.cfg
	File installerskeleton\config\torque_models.cfg
	File installerskeleton\config\wavefield.cfg
	# done with configuration

	; clean cache directory
	Banner::show /NOUNLOAD "cleaning cache directory"
    ; this will empty that directory (but not delete it)
    !insertmacro RemoveFilesAndSubDirs "$DOCUMENTS\Rigs of Rods  ${PRODUCT_VERSION_SHORT}\cache\"
	Banner::destroy
	
	# back to normal installation directory
	SetOutPath $INSTDIR
	
	# add version text file
	!insertmacro WriteToFile "${PRODUCT_VERSION}" "$INSTDIR\version.txt"
	
	# add shortcuts
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
		CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Rigs of Rods.lnk" "$INSTDIR\RoRConfig.exe"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Updater.lnk" "$INSTDIR\updater.exe"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Key Sheet.lnk" "$INSTDIR\keysheet.pdf"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Manual.lnk" "$INSTDIR\Things_you_can_do_in_Rigs_of_Rods.pdf"

		!insertmacro CreateInternetShortcut "$SMPROGRAMS\$StartMenuFolder\Forums" "http://www.rigsofrods.com/forum.php" "$InstDir\cog_go.ico" "0"
		!insertmacro CreateInternetShortcut "$SMPROGRAMS\$StartMenuFolder\Beginners Guide" "http://www.rigsofrods.com/wiki/pages/Beginner%27s_Guide" "$InstDir\forums.ico" "0"	
	!insertmacro MUI_STARTMENU_WRITE_END
	
SectionEnd

Section /o "Content Pack" RoRContentPack
	; only in full
	SectionIn 1
	
	; adds 416 MB
	AddSize 416692
	
	SetShellVarContext current
	SetOutPath "$DOCUMENTS\Rigs of Rods ${PRODUCT_VERSION_SHORT}\"

	!define CONTENT_TARGETFILE "$DOCUMENTS\Rigs of Rods ${PRODUCT_VERSION_SHORT}\content-pack.zip"
	; attention: the useragent is crucial to get the HTTP/302 instead of the fancy webpage
	inetc::get /TIMEOUT=30000 /USERAGENT "wget" "http://download.rigsofrods.com/content/?version=${PRODUCT_VERSION}" "${CONTENT_TARGETFILE}"
	Pop $0 ;Get the return value
	StrCmp $0 "OK" content_pack_unzip
	MessageBox MB_OK|MB_ICONEXCLAMATION "Download of Content Pack failed: $0" /SD IDOK
content_pack_unzip:
	ZipDLL::extractall "${CONTENT_TARGETFILE}" "$DOCUMENTS\Rigs of Rods ${PRODUCT_VERSION_SHORT}\packs"
	Goto content_pack_unzip_end
content_pack_unzip_end:	
	; remove downloaded file
	Delete "${CONTENT_TARGETFILE}"
SectionEnd

Section /o "Multiplayer Server" RoRServer
	; only in full
	SectionIn 1
	
	; adds 1.2 MB
	AddSize 1228
	
	SetOutPath "$INSTDIR"

	!define SERVER_TARGETFILE "$INSTDIR\rorserver.zip"
	; attention: the useragent is crucial to get the HTTP/302 instead of the fancy webpage
	inetc::get /TIMEOUT=30000 /USERAGENT "wget" "http://download.rigsofrods.com/server/?version=${PRODUCT_VERSION}" "${SERVER_TARGETFILE}"
	Pop $0 ;Get the return value
	StrCmp $0 "OK" content_pack_unzip
	MessageBox MB_OK|MB_ICONEXCLAMATION "Download of RoR Server failed: $0" /SD IDOK
content_pack_unzip:
	ZipDLL::extractall "${SERVER_TARGETFILE}" "$INSTDIR"
	Goto content_pack_unzip_end
content_pack_unzip_end:	
	; remove downloaded file
	Delete "${SERVER_TARGETFILE}"
	
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
		CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Multiplayer Server.lnk" "$INSTDIR\servergui.exe"
		!insertmacro CreateInternetShortcut "$SMPROGRAMS\$StartMenuFolder\Server Setup Tutorial" "http://www.rigsofrods.com/wiki/pages/Server_Setup_Tutorial" "$INSTDIR\servergui.exe" "0"	
	!insertmacro MUI_STARTMENU_WRITE_END
	
SectionEnd

; section descriptions
;Language strings
LangString DESC_RoRBaseGame ${LANG_ENGLISH} "Rigs of Rods Base - The main simulation, includes Multiplayer client"
LangString DESC_RoRContentPack ${LANG_ENGLISH} "Rigs of Rods Content Pack - will be downloaded"
LangString DESC_RoRServer ${LANG_ENGLISH} "Rigs of Rods Multiplayer Server - if you want to host Multiplayer games yourself - will be downloaded"

;Assign language strings to sections
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${RoRBaseGame} $(DESC_RoRBaseGame)
	!insertmacro MUI_DESCRIPTION_TEXT ${RoRContentPack} $(DESC_RoRContentPack)
	!insertmacro MUI_DESCRIPTION_TEXT ${RoRServer} $(DESC_RoRServer)
!insertmacro MUI_FUNCTION_DESCRIPTION_END
 
;Section "Modding Tools" SEC03
;    SetOutPath "$INSTDIR"
;    SetOverwrite try
;	File /r /x .svn ..\..\tools\modtool
;SectionEnd

Function "LaunchPostInstallation"
	Exec  "$INSTDIR\rorconfig.exe"
FunctionEnd

Section -Post

	Call InstallDirectX
	Call InstallVisualStudioRuntime
	
	${registerExtension} "$INSTDIR\RoRMeshViewer.exe" ".mesh" "Ogre Mesh"

	; write uninstaller
	WriteUninstaller "$INSTDIR\uninst.exe"
	# only add the uninstall link here
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
		CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\uninst.exe"
	!insertmacro MUI_STARTMENU_WRITE_END

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

	
	WriteRegStr HKLM "${ROR_REGISTRY_ENTRY}" "InstallLocation" "$INSTDIR"

	; add some registry info
	WriteRegStr HKCU "Software\RigsOfRods\" "directory" "$INSTDIR"
	WriteRegStr HKCU "Software\RigsOfRods\" "version" "${PRODUCT_VERSION}"

	WriteRegStr HKCR "rorserver" "" "URL:Rigs of Rods Server"
	WriteRegStr HKCR "rorserver" "Url Protocol" ""
	WriteRegStr HKCR "rorserver\DefaultIcon" "" ""
	WriteRegStr HKCR "rorserver\shell\open\command" "" '"$INSTDIR\RoR.exe" -wd="$INSTDIR" -join="%1"'
	
	; no reboot
	SetRebootFlag false
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
	RMDir /r "$INSTDIR\*.*"   
	RMDir "$INSTDIR"

	!insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
	
	SetShellVarContext current
	Delete "$SMPROGRAMS\$StartMenuFolder\*.*"
	RmDir  "$SMPROGRAMS\$StartMenuFolder"
	
	; remove registry entries
	DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
	DeleteRegKey HKCU "Software\RigsOfRods\"
	DeleteRegKey HKCR "rorserver"
	
	DeleteRegKey HKLM "${ROR_REGISTRY_ENTRY}"
	
	${unregisterExtension} ".mesh" "Ogre Mesh"

	; if upgrading, do not remove user content
	!insertmacro GetParameterValue "/UPGRADE=" "0"
	Pop $0
	StrCmp $0 "1" end

	SetShellVarContext current
	MessageBox MB_YESNO|MB_ICONEXCLAMATION "Remove RoR user content in $DOCUMENTS\Rigs of Rods ${PRODUCT_VERSION_SHORT}?" IDYES removedoc IDNO end
removedoc:
	MessageBox MB_YESNO|MB_ICONEXCLAMATION "Are you really sure to remove all user content in $DOCUMENTS\Rigs of Rods ${PRODUCT_VERSION_SHORT}?" IDYES removedoc2 IDNO end
removedoc2:
	RMDir  "/r" "$DOCUMENTS\Rigs of Rods ${PRODUCT_VERSION_SHORT}"
end:
	SetAutoClose false
SectionEnd
