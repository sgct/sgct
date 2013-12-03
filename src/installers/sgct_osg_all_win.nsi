!include MUI2.nsh
!include EnvVarUpdate.nsh
!include FileAssociation.nsh
!include LogicLib.nsh ;if statements and loops
!include x64.nsh

;Change the following defines to make different installers
!define SGCT_VERSION "2.0.7"
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
!else if "${SGCT_COMPILER}" == "msvc12"		
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

Function .onInit
!if "${ARCH}" == "x64"
	!define SGCT_LIB_PATH "${SGCT_COMPILER}_x64"
	StrCpy $InstDir "$PROGRAMFILES64\SGCT"
!else
	!define SGCT_LIB_PATH "${SGCT_COMPILER}"
	StrCpy $InstDir "$PROGRAMFILES\SGCT"
!endif
FunctionEnd

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
OutFile "..\..\bin\installers\SGCT_${SGCT_VERSION}_${SGCT_COMPILER}_${ARCH}_setup.exe"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\SGCT" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages
!define MUI_COMPONENTSPAGE_NODESC
!define MUI_WELCOMEPAGE_TEXT "Simple Graphics Cluster Toolkit (SGCT) is a static cross-platform C++ library for developing OpenGL applications."
!define MUI_WELCOMEFINISHPAGE_BITMAP "sgct.bmp"
;!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH

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
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\include"
	File /r "..\..\include\"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\lib\${SGCT_LIB_PATH}"
	!if "${SGCT_COMPILER}" == "mingw"
		File "..\..\lib\mingw\libsgct.a"
		File "..\..\lib\mingw\libsgctd.a"
		File "..\..\additional_libs\ALUT\mingw\alut.a"
		File "..\..\additional_libs\ALUT\mingw\alutd.a"
	!else if "${ARCH}" == "x86"
		File "..\..\lib\${SGCT_COMPILER}\sgct.lib"
		File "..\..\lib\${SGCT_COMPILER}\sgctd.lib"
		File "..\..\additional_libs\ALUT\${SGCT_COMPILER}\alut.lib"
		File "..\..\additional_libs\ALUT\${SGCT_COMPILER}\alutd.lib"
	!else if "${ARCH}" == "x64"
		File "..\..\lib\${SGCT_COMPILER}_x64\sgct.lib"
		File "..\..\lib\${SGCT_COMPILER}_x64\sgctd.lib"
		File "..\..\additional_libs\ALUT\${SGCT_COMPILER}_x64\alut.lib"
		File "..\..\additional_libs\ALUT\${SGCT_COMPILER}_x64\alutd.lib"
	!endif
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}"
	File "..\..\license.txt"
	File "..\..\Attribution.txt"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\docs"
	File /r "..\..\docs\html"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\config"
	File /r "..\..\config\"
	
	#add examples
	#============================================
	
	#template
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\sgct_template"
	File "..\..\src\apps\sgct_template\main.cpp"
	File "..\..\src\apps\sgct_template\sgct_template_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\sgct_template\CMakeLists.txt"
	
	#example1
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\spinning_triangle"
	File "..\..\bin\example1\${SGCT_COMPILER}\example1_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\example1\main.cpp"
	File "..\..\src\apps\example1\example1_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\example1\CMakeLists.txt"
	
	#example1 opengl3
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\spinning_triangle_opengl3"
	File "..\..\bin\example1_opengl3\${SGCT_COMPILER}\example1_opengl3_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\example1_opengl3\main.cpp"
	File "..\..\src\apps\example1_opengl3\SimpleFragmentShader.fragmentshader"
	File "..\..\src\apps\example1_opengl3\SimpleVertexShader.vertexshader"
	File "..\..\src\apps\example1_opengl3\example1_opengl3_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\example1_opengl3\CMakeLists.txt"
	
	#texture_example
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\texture_example"
	File "..\..\bin\textureExample\${SGCT_COMPILER}\textureExample_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\textureExample\box.png"
	File "..\..\src\apps\textureExample\main.cpp"
	File "..\..\src\apps\textureExample\textureExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\textureExample\CMakeLists.txt"
	
	#texture_example opengl3
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\texture_example_opengl3"
	File "..\..\bin\textureExample_opengl3\${SGCT_COMPILER}\textureExample_opengl3_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\textureExample_opengl3\box.png"
	File "..\..\src\apps\textureExample_opengl3\main.cpp"
	File "..\..\src\apps\textureExample_opengl3\SimpleFragmentShader.fragmentshader"
	File "..\..\src\apps\textureExample_opengl3\SimpleVertexShader.vertexshader"
	File "..\..\src\apps\textureExample_opengl3\textureExample_opengl3_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\textureExample_opengl3\CMakeLists.txt"
	
	#simple shader
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\simple_shader"
	File "..\..\bin\simpleShaderExample\${SGCT_COMPILER}\simpleShaderExample_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\simpleShaderExample\simple.frag"
	File "..\..\src\apps\simpleShaderExample\simple.vert"
	File "..\..\src\apps\simpleShaderExample\main.cpp"
	File "..\..\src\apps\simpleShaderExample\simpleShaderExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\simpleShaderExample\CMakeLists.txt"
	
	#simple shader opengl3
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\simple_shader_opengl3"
	File "..\..\bin\simpleShaderExample_opengl3\${SGCT_COMPILER}\simpleShaderExample_opengl3_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\simpleShaderExample_opengl3\simple.frag"
	File "..\..\src\apps\simpleShaderExample_opengl3\simple.vert"
	File "..\..\src\apps\simpleShaderExample_opengl3\main.cpp"
	File "..\..\src\apps\simpleShaderExample_opengl3\simpleShaderExample_opengl3_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\simpleShaderExample_opengl3\CMakeLists.txt"
	
	#height mapping example
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\height_mapping"
	File "..\..\bin\heightMappingExample\${SGCT_COMPILER}\heightMappingExample_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\heightMappingExample\heightmap.frag"
	File "..\..\src\apps\heightMappingExample\heightmap.vert"
	File "..\..\src\apps\heightMappingExample\heightmap.png"
	File "..\..\src\apps\heightMappingExample\normalmap.png"
	File "..\..\src\apps\heightMappingExample\main.cpp"
	File "..\..\src\apps\heightMappingExample\fisheye.xml"
	File "..\..\src\apps\heightMappingExample\heightMappingExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\heightMappingExample\CMakeLists.txt"
	
	#height mapping example opengl3
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\height_mapping_opengl3"
	File "..\..\bin\heightMappingExample_opengl3\${SGCT_COMPILER}\heightMappingExample_opengl3_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\heightMappingExample_opengl3\heightmap.frag"
	File "..\..\src\apps\heightMappingExample_opengl3\heightmap.vert"
	File "..\..\src\apps\heightMappingExample_opengl3\heightmap.png"
	File "..\..\src\apps\heightMappingExample_opengl3\normalmap.png"
	File "..\..\src\apps\heightMappingExample_opengl3\main.cpp"
	File "..\..\src\apps\heightMappingExample_opengl3\fisheye.xml"
	File "..\..\src\apps\heightMappingExample_opengl3\heightMappingExample_opengl3_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\heightMappingExample_opengl3\CMakeLists.txt"
	
	#simple navigation
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\simple_navigation"
	File "..\..\bin\simpleNavigationExample\${SGCT_COMPILER}\simpleNavigationExample_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\simpleNavigationExample\main.cpp"
	File "..\..\src\apps\simpleNavigationExample\simpleNavigationExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\simpleNavigationExample\CMakeLists.txt"
	
	#gamepad
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\gamepad"
	File "..\..\bin\gamepadExample\${SGCT_COMPILER}\gamepadExample_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\gamepadExample\main.cpp"
	File "..\..\src\apps\gamepadExample\gamepadExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\gamepadExample\CMakeLists.txt"
	
	#render to texture
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\render_to_texture"
	File "..\..\bin\renderToTexture\${SGCT_COMPILER}\renderToTexture_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\renderToTexture\simple.frag"
	File "..\..\src\apps\renderToTexture\simple.vert"
	File "..\..\src\apps\renderToTexture\main.cpp"
	File "..\..\src\apps\textureExample\box.png"
	File "..\..\src\apps\renderToTexture\renderToTexture_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\renderToTexture\CMakeLists.txt"
	
	#post FX
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\post_fx_example"
	File "..\..\bin\postFXExample\${SGCT_COMPILER}\postFXExample_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\postFXExample\box.png"
	File "..\..\src\apps\postFXExample\main.cpp"
	File "..\..\src\apps\postFXExample\base.vert"
	File "..\..\src\apps\postFXExample\blur.frag"
	File "..\..\src\apps\postFXExample\blur_h.vert"
	File "..\..\src\apps\postFXExample\blur_v.vert"
	File "..\..\src\apps\postFXExample\glow.frag"
	File "..\..\src\apps\postFXExample\threshold.frag"
	File "..\..\src\apps\postFXExample\postFXExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\postFXExample\CMakeLists.txt"
	
	#post FX opengl3
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\post_fx_example_opengl3"
	File "..\..\bin\postFXExample_opengl3\${SGCT_COMPILER}\postFXExample_opengl3_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\src\apps\postFXExample_opengl3\box.png"
	File "..\..\src\apps\postFXExample_opengl3\main.cpp"
	File "..\..\src\apps\postFXExample_opengl3\base.vert"
	File "..\..\src\apps\postFXExample_opengl3\blur.frag"
	File "..\..\src\apps\postFXExample_opengl3\blur_h.vert"
	File "..\..\src\apps\postFXExample_opengl3\blur_v.vert"
	File "..\..\src\apps\postFXExample_opengl3\glow.frag"
	File "..\..\src\apps\postFXExample_opengl3\threshold.frag"
	File "..\..\src\apps\postFXExample_opengl3\SimpleFragmentShader.fragmentshader"
	File "..\..\src\apps\postFXExample_opengl3\SimpleVertexShader.vertexshader"
	File "..\..\src\apps\postFXExample_opengl3\postFXExample_opengl3_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\postFXExample_opengl3\CMakeLists.txt"
	
	#osg examples
	!if ${INC_OSG} == 1
		SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\osg_example"
		File "..\..\bin\osgExample\${SGCT_COMPILER}\osgExample_${SGCT_COMPILER}_${ARCH}.exe"
		File "..\..\src\apps\osgExample\airplane.ive"
		File "..\..\src\apps\osgExample\main.cpp"
		File "..\..\src\apps\osgExample\osgExample_${SGCT_COMPILER}.${PRJ_SUFFIX}"
		File "..\..\src\apps\osgExample\CMakeLists.txt"
		
		SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\osg_exampleRTT"
		File "..\..\bin\osgExampleRTT\${SGCT_COMPILER}\osgExampleRTT_${SGCT_COMPILER}_${ARCH}.exe"
		File "..\..\src\apps\osgExample\airplane.ive"
		File "..\..\src\apps\osgExampleRTT\main.cpp"
		File "..\..\src\apps\osgExampleRTT\RenderToTexture.cpp"
		File "..\..\src\apps\osgExampleRTT\RenderToTexture.h"
		File "..\..\src\apps\osgExampleRTT\osgExampleRTT_${SGCT_COMPILER}.${PRJ_SUFFIX}"
		File "..\..\src\apps\osgExample\CMakeLists.txt"
	!endif
	
	#remote
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\remote_app"
	File "..\..\bin\remote_app\${SGCT_COMPILER}\remote_app_${SGCT_COMPILER}_${ARCH}.exe"
	File "..\..\bin\remote_app\SGCTRemote.exe"
	File "..\..\src\apps\SGCTRemote\main.cpp"
	File "..\..\src\apps\SGCTRemote\single_remote.xml"
	File "..\..\src\apps\SGCTRemote\remote_app_${SGCT_COMPILER}.${PRJ_SUFFIX}"
	File "..\..\src\apps\SGCTRemote\CMakeLists.txt"
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\remote_app\SGCTRemote_GUI"
	File /r "..\..\src\apps\SGCTRemote\SGCTRemote_GUI\Properties"
	File "..\..\src\apps\SGCTRemote\SGCTRemote_GUI\Form1.cs"
	File "..\..\src\apps\SGCTRemote\SGCTRemote_GUI\Form1.Designer.cs"
	File "..\..\src\apps\SGCTRemote\SGCTRemote_GUI\Form1.resx"
	File "..\..\src\apps\SGCTRemote\SGCTRemote_GUI\NetworkManager.cs"
	File "..\..\src\apps\SGCTRemote\SGCTRemote_GUI\Program.cs"
	File "..\..\src\apps\SGCTRemote\SGCTRemote_GUI\SGCTRemote.csproj"
SectionEnd

Section "SGCT environment variable"
	;remove if set
	DeleteRegValue ${env_hklm} SGCT_ROOT_DIR
	
	${EnvVarUpdate} $0 "SGCT_ROOT_DIR" "P" "HKLM" "$INSTDIR\SGCT_${SGCT_VERSION}"
	
	;update env vars
	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
	
	; reboot after install
	SetRebootFlag true
SectionEnd

Section "Start Menu Shortcuts"
  #all users
  SetShellVarContext all
  #current user
  #SetShellVarContext current
  
  CreateDirectory "$SMPROGRAMS\SGCT"
  #not that good in the lab to have an uninstall shortcut
  #CreateShortCut "$SMPROGRAMS\SGCT\Uninstall.lnk" "$INSTDIR\uninstall.exe" ""
  
  #add examples
  SetOutPath $INSTDIR
  CreateDirectory "$SMPROGRAMS\SGCT\examples"
  
  CreateShortCut "$SMPROGRAMS\SGCT\C-Student_wiki.lnk" "$INSTDIR\C-Student_wiki.url" "" "$INSTDIR\C_transparent.ico"
  CreateShortCut "$SMPROGRAMS\SGCT\SGCT_tutorials.lnk" "$INSTDIR\SGCT_tutorials.url" "" "$INSTDIR\C_transparent.ico"
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\docs"
  CreateShortCut "$SMPROGRAMS\SGCT\SGCT_Documentation.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\docs\html\index.html" ""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\spinning_triangle"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\spinning_triangle.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\spinning_triangle\example1_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  CreateShortCut "$SMPROGRAMS\SGCT\examples\spinning_triangle_two_win.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\spinning_triangle\example1_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single_two_win.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\spinning_triangle_opengl3"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\spinning_triangle_opengl3.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\spinning_triangle_opengl3\example1_opengl3_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  CreateShortCut "$SMPROGRAMS\SGCT\examples\spinning_triangle_opengl3_two_win.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\spinning_triangle_opengl3\example1_opengl3_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\ssingle_two_win.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\texture_example"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\texture_example.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\texture_example\textureExample_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\textureExample_opengl3"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\textureExample_opengl3.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\textureExample_opengl3\textureExample_opengl3_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\simple_shader"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\simple_shader.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\simple_shader\simpleShaderExample_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\height_mapping"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\height_mapping.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\height_mapping\heightMappingExample_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  CreateShortCut "$SMPROGRAMS\SGCT\examples\height_mapping_fisheye.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\height_mapping\heightMappingExample_${SGCT_COMPILER}_${ARCH}.exe" "-config fisheye.xml"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\height_mapping_two_win.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\height_mapping\heightMappingExample_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single_two_win.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\height_mapping_opengl3"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\height_mapping_opengl3.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\height_mapping_opengl3\heightMappingExample_opengl3_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  CreateShortCut "$SMPROGRAMS\SGCT\examples\height_mapping_opengl3_fisheye.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\height_mapping_opengl3\heightMappingExample_opengl3_${SGCT_COMPILER}_${ARCH}.exe" "-config fisheye.xml"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\height_mapping_opengl3_two_win.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\height_mapping_opengl3\heightMappingExample_opengl3_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single_two_win.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\simple_navigation"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\simple_navigation.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\simple_navigation\simpleNavigationExample_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  CreateShortCut "$SMPROGRAMS\SGCT\examples\simple_navigation_fisheye.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\simple_navigation\simpleNavigationExample_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single_fisheye.xml$\""
  CreateShortCut "$SMPROGRAMS\SGCT\examples\simple_navigation_fisheye_FXAA.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\simple_navigation\simpleNavigationExample_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single_fisheye_fxaa.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\gamepad"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\gamepad.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\gamepad\gamepadExample_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\render_to_texture"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\render_to_texture.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\render_to_texture\renderToTexture_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\post_fx_example"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\post_fx_example.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\post_fx_example\postFXExample_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\post_fx_example_opengl3"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\post_fx_example_opengl3.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\post_fx_example_opengl3\postFXExample_opengl3_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  !if ${INC_OSG} == 1
	  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\osg_example"
	  CreateShortCut "$SMPROGRAMS\SGCT\examples\osg_example.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\osg_example\osgExample_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
	  
	  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\osg_exampleRTT"
	  CreateShortCut "$SMPROGRAMS\SGCT\examples\osg_exampleRTT.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\osg_exampleRTT\osgExampleRTT_${SGCT_COMPILER}_${ARCH}.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  !endif
  
  CreateDirectory "$SMPROGRAMS\SGCT\examples\remote"
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}\examples\remote_app"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\remote\remote_app.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\remote_app\remote_app_${SGCT_COMPILER}_${ARCH}.exe" "-config single_remote.xml"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\remote\SGCTRemote.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}\examples\remote_app\SGCTRemote_${ARCH}.exe" ""
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