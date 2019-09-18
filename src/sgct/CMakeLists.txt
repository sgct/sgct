set(HEADER_FILES
  ${PROJECT_SOURCE_DIR}/include/sgct.h
  ${PROJECT_SOURCE_DIR}/include/sgct/BaseViewport.h
  ${PROJECT_SOURCE_DIR}/include/sgct/ClusterManager.h
  ${PROJECT_SOURCE_DIR}/include/sgct/CorrectionMesh.h
  ${PROJECT_SOURCE_DIR}/include/sgct/Engine.h
  ${PROJECT_SOURCE_DIR}/include/sgct/FisheyeProjection.h
  ${PROJECT_SOURCE_DIR}/include/sgct/Font.h
  ${PROJECT_SOURCE_DIR}/include/sgct/FontManager.h
  ${PROJECT_SOURCE_DIR}/include/sgct/freetype.h
  ${PROJECT_SOURCE_DIR}/include/sgct/Frustum.h
  ${PROJECT_SOURCE_DIR}/include/sgct/Image.h
  ${PROJECT_SOURCE_DIR}/include/sgct/MessageHandler.h
  ${PROJECT_SOURCE_DIR}/include/sgct/NetworkManager.h
  ${PROJECT_SOURCE_DIR}/include/sgct/NonLinearProjection.h
  ${PROJECT_SOURCE_DIR}/include/sgct/OffScreenBuffer.h
  ${PROJECT_SOURCE_DIR}/include/sgct/ogl_headers.h
  ${PROJECT_SOURCE_DIR}/include/sgct/PostFX.h
  ${PROJECT_SOURCE_DIR}/include/sgct/ReadConfig.h
  ${PROJECT_SOURCE_DIR}/include/sgct/ScreenCapture.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTConfig.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTMpcdi.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTMutexManager.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTNetwork.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTNode.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTOpenVR.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTProjection.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTProjectionPlane.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTSettings.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTTracker.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTTrackingDevice.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTTrackingManager.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTUser.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTVersion.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTWindow.h
  ${PROJECT_SOURCE_DIR}/include/sgct/Shader.h
  ${PROJECT_SOURCE_DIR}/include/sgct/ShaderData.h
  ${PROJECT_SOURCE_DIR}/include/sgct/ShaderManager.h
  ${PROJECT_SOURCE_DIR}/include/sgct/ShaderProgram.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SharedData.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SharedDataTypes.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SphericalMirrorProjection.h
  ${PROJECT_SOURCE_DIR}/include/sgct/SpoutOutputProjection.h
  ${PROJECT_SOURCE_DIR}/include/sgct/Statistics.h
  ${PROJECT_SOURCE_DIR}/include/sgct/TextureManager.h
  ${PROJECT_SOURCE_DIR}/include/sgct/Touch.h
  ${PROJECT_SOURCE_DIR}/include/sgct/Viewport.h
  ${PROJECT_SOURCE_DIR}/include/sgct/helpers/SGCTPortedFunctions.h
  ${PROJECT_SOURCE_DIR}/include/sgct/helpers/SGCTStringFunctions.h
  ${PROJECT_SOURCE_DIR}/include/sgct/helpers/SGCTVertexData.h
  ${PROJECT_SOURCE_DIR}/include/sgct/shaders/SGCTInternalFisheyeShaders.h
  ${PROJECT_SOURCE_DIR}/include/sgct/shaders/SGCTInternalFisheyeShaders_cubic.h
  ${PROJECT_SOURCE_DIR}/include/sgct/shaders/SGCTInternalFisheyeShaders_modern.h
  ${PROJECT_SOURCE_DIR}/include/sgct/shaders/SGCTInternalFisheyeShaders_modern_cubic.h
  ${PROJECT_SOURCE_DIR}/include/sgct/shaders/SGCTInternalShaders.h
  ${PROJECT_SOURCE_DIR}/include/sgct/shaders/SGCTInternalShaders_modern.h
  ${PROJECT_SOURCE_DIR}/include/sgct/shaders/SGCTInternalSphericalProjectionShaders.h
  ${PROJECT_SOURCE_DIR}/include/sgct/shaders/SGCTInternalSphericalProjectionShaders_modern.h
  ${PROJECT_SOURCE_DIR}/include/sgct/utils/SGCTBox.h
  ${PROJECT_SOURCE_DIR}/include/sgct/utils/SGCTDome.h
  ${PROJECT_SOURCE_DIR}/include/sgct/utils/SGCTDomeGrid.h
  ${PROJECT_SOURCE_DIR}/include/sgct/utils/SGCTPlane.h
  ${PROJECT_SOURCE_DIR}/include/sgct/utils/SGCTSphere.h
)

set(SOURCE_FILES
  BaseViewport.cpp
  ClusterManager.cpp
  CorrectionMesh.cpp
  Engine.cpp
  FisheyeProjection.cpp
  Font.cpp
  FontManager.cpp
  freetype.cpp
  Image.cpp
  MessageHandler.cpp
  NetworkManager.cpp
  NonLinearProjection.cpp
  OffScreenBuffer.cpp
  PostFX.cpp
  ReadConfig.cpp
  ScreenCapture.cpp
  SGCTMpcdi.cpp
  SGCTMutexManager.cpp
  SGCTNetwork.cpp
  SGCTNode.cpp
  SGCTOpenVR.cpp
  SGCTProjection.cpp
  SGCTProjectionPlane.cpp
  SGCTSettings.cpp
  SGCTTracker.cpp
  SGCTTrackingDevice.cpp
  SGCTTrackingManager.cpp
  SGCTUser.cpp
  SGCTVersion.cpp
  SGCTWindow.cpp
  Shader.cpp
  ShaderManager.cpp
  ShaderProgram.cpp
  SharedData.cpp
  SharedDataTypes.cpp
  SphericalMirrorProjection.cpp
  SpoutOutputProjection.cpp
  Statistics.cpp
  TextureManager.cpp
  Touch.cpp
  Viewport.cpp
  utils/SGCTBox.cpp
  utils/SGCTDome.cpp
  utils/SGCTDomeGrid.cpp
  utils/SGCTPlane.cpp
  utils/SGCTSphere.cpp
)

configure_file(
  ${PROJECT_SOURCE_DIR}/src/SGCTConfig.h.in
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTConfig.h @ONLY
)
configure_file(
  ${PROJECT_SOURCE_DIR}/src/SGCTVersion.h.in
  ${PROJECT_SOURCE_DIR}/include/sgct/SGCTVersion.h @ONLY
)

add_library(sgct ${SOURCE_FILES} ${HEADER_FILES})
set_compile_options(sgct)

target_include_directories(sgct PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(sgct SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/ext/include)
target_compile_definitions(sgct PUBLIC GLEW_STATIC GLEW_NO_GLU)

target_link_libraries(sgct
  PRIVATE
    glew glfw png16_static turbojpeg-static tinyxml2static zlibstatic miniziplibstatic vrpn
)

if (SGCT_FREETYPE_SUPPORT)
  target_compile_definitions(sgct PUBLIC SGCT_HAS_TEXT)
  target_include_directories(sgct SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/ext/freetype2/include)
  target_link_libraries(sgct PUBLIC freetype)
endif ()

if (SGCT_SPOUT_SUPPORT)
  target_compile_definitions(sgct PUBLIC SGCT_HAS_SPOUT)
  target_include_directories(sgct SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/ext/spout)
  target_link_libraries(sgct PUBLIC ${PROJECT_SOURCE_DIR}/ext/spout/SpoutLibrary.lib)

  get_target_property(tmp sgct DYNAMIC_LIBS)
  if (NOT tmp)
    set(tmp "")
  endif ()

  set_target_properties(sgct
    PROPERTIES DYNAMIC_LIBS
    "${tmp};${PROJECT_SOURCE_DIR}/ext/spout/SpoutLibrary.dll"
  )
endif ()

if (SGCT_OPENVR_SUPPORT)
  target_compile_definitions(sgct PUBLIC SGCT_HAS_OPENVR)
  target_include_directories(sgct SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/ext/openvr/headers)
endif ()

find_package(OpenGL REQUIRED)
target_link_libraries(sgct PUBLIC ${OPENGL_gl_LIBRARY})

if (WIN32)
  target_link_libraries(sgct PRIVATE ws2_32)
elseif (APPLE)
  find_library(COCOA_LIBRARY Cocoa REQUIRED)
  find_library(IOKIT_LIBRARY IOKit REQUIRED)
  find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)

  target_link_libraries(sgct PRIVATE
    ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY}
  )
else() # Linux
  find_package(X11 REQUIRED)
  find_package(Threads REQUIRED)

  target_link_libraries(sgct PRIVATE
    ${X11_X11_LIB} ${X11_Xrandr_LIB} ${X11_Xinerama_LIB} ${X11_Xinput_LIB}
    ${X11_Xxf86vm_LIB} ${X11_Xcursor_LIB}
  )
endif ()

target_include_directories(sgct SYSTEM PUBLIC
    ${PROJECT_SOURCE_DIR}/ext/glfw/include
    ${PROJECT_SOURCE_DIR}/ext/tinyxml2
)

target_include_directories(sgct SYSTEM PUBLIC
  ${PROJECT_BINARY_DIR}/ext/vrpn
  ${PROJECT_SOURCE_DIR}/ext/vrpn
  ${PROJECT_BINARY_DIR}/ext/lpng
  ${PROJECT_SOURCE_DIR}/ext/lpng
  ${PROJECT_SOURCE_DIR}/ext/zlib
  ${PROJECT_BINARY_DIR}/ext/lpng/zlib
  ${PROJECT_SOURCE_DIR}/ext/lpng/zlib
  ${PROJECT_SOURCE_DIR}/ext/zlib/contrib/minizip
  ${PROJECT_BINARY_DIR}/ext/libjpeg-turbo
  ${PROJECT_SOURCE_DIR}/ext/libjpeg-turbo
)
