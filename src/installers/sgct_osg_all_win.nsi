!include MUI2.nsh
!include EnvVarUpdate.nsh
!include FileAssociation.nsh
!include LogicLib.nsh ;if statements and loops

;Change the following defines to make different installers
!define SGCT_VERSION "1.2.0"
!define SGCT_COMPILER "msvc10"
!define ARCH "x86"
!define OSG_VERSION "3.0.1"
!define INC_OSG 1

!if "${SGCT_COMPILER}" == "msvc9"
	!define PRJ_SUFFIX "vcproj"
	!define OSG_BIN_DIR "D:\bin\osg\OpenSceneGraph-${OSG_VERSION}-VS9.0.30729-${ARCH}-release-12741\"
!else if "${SGCT_COMPILER}" == "msvc10"
	!define PRJ_SUFFIX "vcxproj"
	!define OSG_BIN_DIR "D:\bin\osg\OpenSceneGraph-${OSG_VERSION}-VS10.0.30319-${ARCH}-release-12741\"
!else if "${SGCT_COMPILER}" == "msvc11"		
	!define PRJ_SUFFIX "vcxproj"
!else	
	!define PRJ_SUFFIX "cbp"
	!define OSG_BIN_DIR "D:\bin\osg\OpenSceneGraph-${OSG_VERSION}-MinGW\"
!endif

!define OSG_DATA_DIR "D:\bin\osg\OpenSceneGraph-Data-3.0.0\"

; The name of the installer
Name "SGCT ${SGCT_VERSION} ${SGCT_COMPILER} ${ARCH} installer"

!define REALMSG "$\nOriginal non-restricted account type: $2"
!define env_hklm 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
!define MUI_COMPONENTSPAGE_NODESC
!define MUI_WELCOMEPAGE_TEXT "Simple Graphics Cluster Toolkit (SGCT) is a static cross-platform C++ library for developing OpenGL applications."
!define MUI_WELCOMEFINISHPAGE_BITMAP "sgct.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH

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
OutFile "..\..\bin\installers\SGCT_${SGCT_VERSION}_${SGCT_COMPILER}_setup.exe"

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
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "License.rtf"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

;--------------------------------

Section "SGCT ${SGCT_VERSION} ${SGCT_COMPILER} ${ARCH}"
	SectionIn RO
	
	; Write the installation path into the registry
	WriteRegStr HKLM "SOFTWARE\SGCT" "Install_Dir" "$INSTDIR"
	  
	; Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT" "DisplayName" "SGCT ${SGCT_VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT" "NoRepair" 1
	
	SetOutPath $INSTDIR
	writeUninstaller "$INSTDIR\uninstall.exe"
	
	File C-Student_wiki.url
	File SGCT_tutorials.url
	File C_transparent.ico
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\include"
	File /r "..\..\include\"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\lib\${SGCT_COMPILER}"
	!if "${SGCT_COMPILER}" == "mingw"
		File "..\..\lib\mingw\libsgct32.a"
		File "..\..\lib\mingw\libsgct32_d.a"
	!else
		File "..\..\lib\${SGCT_COMPILER}\sgct32.lib"
		File "..\..\lib\${SGCT_COMPILER}\sgct32_d.lib"
	!endif
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}"
	File "..\..\license.txt"
	File "..\..\Attribution.txt"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\doc"
	File "..\..\docs\latex\refman.pdf"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\config"
	File /r "..\..\config\"
	
	#add examples
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\sgct_template"
	File "..\..\src\apps\sgct_template\main.cpp"
	File "..\..\src\apps\sgct_template\sgct_template_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\spinning_triangle"
	File "..\..\bin\example1\${SGCT_COMPILER}\example1_${SGCT_COMPILER}.exe"
	File "..\..\src\apps\example1\main.cpp"
	File "..\..\src\apps\example1\example1_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\texture_example"
	File "..\..\bin\textureExample\${SGCT_COMPILER}\textureExample_${SGCT_COMPILER}.exe"
	File "..\..\src\apps\textureExample\box.png"
	File "..\..\src\apps\textureExample\main.cpp"
	File "..\..\src\apps\textureExample\textureExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\simple_shader"
	File "..\..\bin\simpleShaderExample\${SGCT_COMPILER}\simpleShaderExample_${SGCT_COMPILER}.exe"
	File "..\..\src\apps\simpleShaderExample\simple.frag"
	File "..\..\src\apps\simpleShaderExample\simple.vert"
	File "..\..\src\apps\simpleShaderExample\main.cpp"
	File "..\..\src\apps\simpleShaderExample\simpleShaderExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\height_mapping"
	File "..\..\bin\heightMappingExample\${SGCT_COMPILER}\heightMappingExample_${SGCT_COMPILER}.exe"
	File "..\..\src\apps\heightMappingExample\heightmap.frag"
	File "..\..\src\apps\heightMappingExample\heightmap.vert"
	File "..\..\src\apps\heightMappingExample\heightmap.png"
	File "..\..\src\apps\heightMappingExample\normalmap.png"
	File "..\..\src\apps\heightMappingExample\main.cpp"
	File "..\..\src\apps\heightMappingExample\heightMappingExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\simple_navigation"
	File "..\..\bin\simpleNavigationExample\${SGCT_COMPILER}\simpleNavigationExample_${SGCT_COMPILER}.exe"
	File "..\..\src\apps\simpleNavigationExample\main.cpp"
	File "..\..\src\apps\simpleNavigationExample\simpleNavigationExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\gamepad"
	File "..\..\bin\gamepadExample\${SGCT_COMPILER}\gamepadExample_${SGCT_COMPILER}.exe"
	File "..\..\src\apps\gamepadExample\main.cpp"
	File "..\..\src\apps\gamepadExample\gamepadExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\post_processing"
	File "..\..\bin\postProcessing\${SGCT_COMPILER}\postProcessing_${SGCT_COMPILER}.exe"
	File "..\..\src\apps\postProcessing\simple.frag"
	File "..\..\src\apps\postProcessing\simple.vert"
	File "..\..\src\apps\postProcessing\main.cpp"
	File "..\..\src\apps\textureExample\box.png"
	File "..\..\src\apps\postProcessing\postProcessing_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	
	!if ${INC_OSG} == 1
		SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\osg_example"
		File "..\..\bin\osgExample\${SGCT_COMPILER}\osgExample_${SGCT_COMPILER}.exe"
		File "..\..\src\apps\osgExample\airplane.ive"
		File "..\..\src\apps\osgExample\main.cpp"
		File "..\..\src\apps\osgExample\osgExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
		
		SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\osg_exampleRTT"
		File "..\..\bin\osgExampleRTT\${SGCT_COMPILER}\osgExampleRTT_${SGCT_COMPILER}.exe"
		File "..\..\src\apps\osgExample\airplane.ive"
		File "..\..\src\apps\osgExampleRTT\main.cpp"
		File "..\..\src\apps\osgExampleRTT\RenderToTexture.cpp"
		File "..\..\src\apps\osgExampleRTT\RenderToTexture.h"
		File "..\..\src\apps\osgExampleRTT\osgExampleRTT_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	!endif
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\remote_app"
	File "..\..\bin\remote_app\${SGCT_COMPILER}\remote_app_${SGCT_COMPILER}.exe"
	File "..\..\bin\remote_app\SGCTRemote.exe"
	File "..\..\src\apps\SGCTRemote\main.cpp"
	File "..\..\src\apps\SGCTRemote\single_remote.xml"
	File "..\..\src\apps\SGCTRemote\remote_app_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File /r "..\..\src\apps\SGCTRemote\SGCTRemote\"
SectionEnd

Section "SGCT environment variable"
	;remove if set
	DeleteRegValue ${env_hklm} SGCT_ROOT_DIR
	
	${EnvVarUpdate} $0 "SGCT_ROOT_DIR" "P" "HKLM" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}"
	
	;update env vars
	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
	
	; reboot after install
	SetRebootFlag true
SectionEnd

Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\SGCT"
  CreateShortCut "$SMPROGRAMS\SGCT\Uninstall.lnk" "$INSTDIR\uninstall.exe" ""
  
  #add examples
  SetOutPath $INSTDIR
  CreateDirectory "$SMPROGRAMS\SGCT\examples"
  
  CreateShortCut "$SMPROGRAMS\SGCT\C-Student_wiki.lnk" "$INSTDIR\C-Student_wiki.url" "" "$INSTDIR\C_transparent.ico"
  CreateShortCut "$SMPROGRAMS\SGCT\SGCT_tutorials.lnk" "$INSTDIR\SGCT_tutorials.url" "" "$INSTDIR\C_transparent.ico"
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\doc"
  CreateShortCut "$SMPROGRAMS\SGCT\SGCT_Documentation.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\doc\refman.pdf" ""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\spinning_triangle"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\spinning_triangle.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\spinning_triangle\example1_${SGCT_COMPILER}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\texture_example"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\texture_example.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\texture_example\textureExample_${SGCT_COMPILER}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\simple_shader"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\simple_shader.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\simple_shader\simpleShaderExample_${SGCT_COMPILER}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\height_mapping"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\height_mapping.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\height_mapping\heightMappingExample_${SGCT_COMPILER}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\simple_navigation"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\simple_navigation.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\simple_navigation\simpleNavigationExample_${SGCT_COMPILER}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  CreateShortCut "$SMPROGRAMS\SGCT\examples\simple_navigation_fisheye.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\simple_navigation\simpleNavigationExample_${SGCT_COMPILER}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single_fisheye.xml$\""
  CreateShortCut "$SMPROGRAMS\SGCT\examples\simple_navigation_fisheye_FXAA.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\simple_navigation\simpleNavigationExample_${SGCT_COMPILER}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single_fisheye.xml$\" --FXAA"
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\gamepad"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\gamepad.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\gamepad\gamepadExample_${SGCT_COMPILER}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\post_processing"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\post_processing.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\post_processing\postProcessing_${SGCT_COMPILER}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  !if ${INC_OSG} == 1
	  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\osg_example"
	  CreateShortCut "$SMPROGRAMS\SGCT\examples\osg_example.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\osg_example\osgExample_${SGCT_COMPILER}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
	  
	  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\osg_exampleRTT"
	  CreateShortCut "$SMPROGRAMS\SGCT\examples\osg_exampleRTT.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\osg_exampleRTT\osgExampleRTT_${SGCT_COMPILER}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  !endif
  
  CreateDirectory "$SMPROGRAMS\SGCT\examples\remote"
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\remote_app"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\remote\remote_app.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\remote_app\remote_app_${SGCT_COMPILER}.exe" "-config single_remote.xml"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\remote\SGCTRemote.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_${ARCH}\examples\remote_app\SGCTRemote.exe" ""
SectionEnd

!if ${INC_OSG} == 1
Section "OSG ${OSG_VERSION} ${SGCT_COMPILER} ${ARCH}"
	SetOutPath "$INSTDIR\osg\osg_${OSG_VERSION}_${SGCT_COMPILER}_${ARCH}"
	File /r "${OSG_BIN_DIR}"
	
	SetOutPath "$INSTDIR\osg\osg_${OSG_VERSION}_${SGCT_COMPILER}_${ARCH}\data"
	File /r "${OSG_DATA_DIR}"
	
	${registerExtension} "$INSTDIR\osg\osg_${OSG_VERSION}_${SGCT_COMPILER}_${ARCH}\bin\osgviewer.exe" ".osg" "OSG file"
	${registerExtension} "$INSTDIR\osg\osg_${OSG_VERSION}_${SGCT_COMPILER}_${ARCH}\bin\osgviewer.exe" ".osgb" "OSG bin file"
	${registerExtension} "$INSTDIR\osg\osg_${OSG_VERSION}_${SGCT_COMPILER}_${ARCH}\bin\osgviewer.exe" ".osgt" "OSG text file"
	${registerExtension} "$INSTDIR\osg\osg_${OSG_VERSION}_${SGCT_COMPILER}_${ARCH}\bin\osgviewer.exe" ".ive" "OSG bin file"
SectionEnd

Section "OSG environment variables"
	;remove if set
	DeleteRegValue ${env_hklm} OSG_PATH
    DeleteRegValue ${env_hklm} OSG_ROOT
    DeleteRegValue ${env_hklm} OSGHOME
	DeleteRegValue ${env_hklm} OSG_FILE_PATH
	
	${EnvVarUpdate} $0 "OSG_ROOT" "P" "HKLM" "$INSTDIR\osg\osg_${OSG_VERSION}_${SGCT_COMPILER}_${ARCH}"
	${EnvVarUpdate} $0 "OSG_PATH" "P" "HKLM" "%OSG_ROOT%\bin"
    ${EnvVarUpdate} $0 "OSGHOME" "P" "HKLM" "%OSG_ROOT%"
	${EnvVarUpdate} $0 "OSG_FILE_PATH" "P" "HKLM" "$INSTDIR\osg\osg_${OSG_VERSION}_${SGCT_COMPILER}_${ARCH}\data"
  
    ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "%OSG_ROOT%\bin"
	
	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
	
	; reboot after install
	SetRebootFlag true
SectionEnd
!endif
;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT"
  DeleteRegKey HKLM "SOFTWARE\SGCT"
  
  ; delete variable
  DeleteRegValue ${env_hklm} SGCT_ROOT_DIR
  
  !if ${INC_OSG} == 1
	  DeleteRegValue ${env_hklm} OSG_PATH
	  DeleteRegValue ${env_hklm} OSG_ROOT
	  DeleteRegValue ${env_hklm} OSGHOME
	  DeleteRegValue ${env_hklm} OSG_FILE_PATH
		
	  ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "%OSG_ROOT%\bin"
	  
	  ${unregisterExtension} ".osg" "OSG file"
	  ${unregisterExtension} ".osgt" "OSG text file"
	  ${unregisterExtension} ".osgb" "OSG bin file"
	  ${unregisterExtension} ".ive" "OSG bin file"
  !endif
  
  ; make sure windows knows about the change
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
	
  Delete $INSTDIR\uninstall.exe

  ; Remove directories used
  RMDir /r "$SMPROGRAMS\SGCT"
  RMDir /r "$INSTDIR"

SectionEnd