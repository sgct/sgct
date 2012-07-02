!include MUI2.nsh
!include EnvVarUpdate.nsh
!include FileAssociation.nsh

; The name of the installer
Name "SGCT 0.8 installer"

!define REALMSG "$\nOriginal non-restricted account type: $2"
!define env_hklm 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'

Section
	ClearErrors
	UserInfo::GetName
	
	Pop $0
	UserInfo::GetAccountType
	Pop $1
	# GetOriginalAccountType will check the tokens of the original user of the
	# current thread/process. If the user tokens were elevated or limited for
	# this process, GetOriginalAccountType will return the non-restricted
	# account type.
	# On Vista with UAC, for example, this is not the same value when running
	# with `RequestExecutionLevel user`. GetOriginalAccountType will return
	# "admin" while GetAccountType will return "user".
	UserInfo::GetOriginalAccountType
	Pop $2
	StrCmp $1 "Admin" 0 +2
		Goto done
	StrCmp $1 "Power" 0 +3
		MessageBox MB_OK "Administrator permission is required to install this software.$\nThe installer will now exit."
		Quit
	StrCmp $1 "User" 0 +3
		MessageBox MB_OK "Administrator permission is required to install this software.$\nThe installer will now exit."
		Quit
	StrCmp $1 "Guest" 0 +3
		MessageBox MB_OK "Administrator permission is required to install this software.$\nThe installer will now exit."
		Quit
	MessageBox MB_OK "Administrator permission is required to install this software.$\nThe installer will now exit."
		Quit
		
	done:
SectionEnd

; The file to write
OutFile "..\..\bin\installers\SGCT_msvc10_setup.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\SGCT"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\SGCT" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages

!define MUI_ICON "C_transparent.ico"

!insertmacro MUI_PAGE_LICENSE "License.rtf"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

;--------------------------------

Section "SGCT 0.8"
	SectionIn RO
	
	; Write the installation path into the registry
	WriteRegStr HKLM "SOFTWARE\SGCT" "Install_Dir" "$INSTDIR"
	  
	; Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT" "DisplayName" "SGCT 0.8"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT" "NoRepair" 1
	
	# Uninstaller - See function un.onInit and section "uninstall" for configuration
	writeUninstaller "$INSTDIR\uninstall.exe"
	
	SetOutPath "$INSTDIR\SGCT-0.8\include"
	File /r "..\..\include\"
	
	SetOutPath "$INSTDIR\SGCT-0.8\lib"
	File "..\..\lib\msvc10\sgct32.lib"
	File "..\..\lib\msvc10\sgct32_d.lib"
	
	SetOutPath "$INSTDIR\SGCT-0.8\config"
	File /r "..\..\config\"
SectionEnd

Section "SGCT environment variable"
	;remove if set
	DeleteRegValue ${env_hklm} SGCT_ROOT_DIR
	
	;update env vars
	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
	
	${EnvVarUpdate} $0 "SGCT_ROOT_DIR" "A" "HKLM" "$INSTDIR\SGCT-0.8"
SectionEnd

Section "SGCT examples"
	SetOutPath "$INSTDIR\SGCT-0.8\examples\spinning_triangle"
	File "..\..\bin\example1\msvc10\example1_msvc10.exe"
	
	
SectionEnd

Section "OSG 3.0.1"
	SetOutPath "$INSTDIR\osg\osg-3.0.1"
	File /r "D:\bin\osg\OpenSceneGraph-3.0.1-VS10.0.30319-x86-release-12741\"
	
	${registerExtension} "$INSTDIR\osg\osg-3.0.1\bin\osgviewer.exe" ".osg" "OSG file"
	${registerExtension} "$INSTDIR\osg\osg-3.0.1\bin\osgviewer.exe" ".osgb" "OSG bin file"
	${registerExtension} "$INSTDIR\osg\osg-3.0.1\bin\osgviewer.exe" ".osgt" "OSG text file"
	${registerExtension} "$INSTDIR\osg\osg-3.0.1\bin\osgviewer.exe" ".ive" "OSG bin file"
SectionEnd

Section "OSG environment variables"
	;remove if set
	DeleteRegValue ${env_hklm} OSG_PATH
    DeleteRegValue ${env_hklm} OSG_ROOT
    DeleteRegValue ${env_hklm} OSGHOME
	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
	
	${EnvVarUpdate} $0 "OSG_ROOT" "A" "HKLM" "$INSTDIR\osg\osg-3.0.1"
	${EnvVarUpdate} $0 "OSG_PATH" "A" "HKLM" "%OSG_ROOT%\bin"
    ${EnvVarUpdate} $0 "OSGHOME" "A" "HKLM" "%OSG_ROOT%"
  
    ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "%OSG_ROOT%\bin"
SectionEnd

Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\SGCT"
  CreateShortCut "$SMPROGRAMS\SGCT\Uninstall.lnk" "$INSTDIR\uninstall.exe" ""
  
  #add examples
  SetOutPath $INSTDIR
  CreateDirectory "$SMPROGRAMS\SGCT\examples"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\spinning_triangle.lnk" "$INSTDIR\SGCT-0.8\examples\spinning_triangle\example1_msvc10.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT"
  DeleteRegKey HKLM "SOFTWARE\SGCT"
  
  ; delete variable
  DeleteRegValue ${env_hklm} SGCT_ROOT_DIR
  DeleteRegValue ${env_hklm} OSG_PATH
  DeleteRegValue ${env_hklm} OSG_ROOT
  DeleteRegValue ${env_hklm} OSGHOME
  
  ; make sure windows knows about the change
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
	
  ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "%OSG_ROOT%\bin"
  
  ${unregisterExtension} ".osg" "OSG file"
  ${unregisterExtension} ".osgt" "OSG text file"
  ${unregisterExtension} ".osgb" "OSG bin file"
  ${unregisterExtension} ".ive" "OSG bin file"
	
  Delete $INSTDIR\uninstall.exe

  ; Remove directories used
  RMDir /r "$SMPROGRAMS\SGCT"
  RMDir /r "$INSTDIR"

SectionEnd