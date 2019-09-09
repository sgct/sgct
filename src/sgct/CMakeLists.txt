set(HEADER_FILES
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

source_group("Source Files" FILES ${SOURCE_FILES})

add_library(sgct ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(sgct PUBLIC ${PROJECT_SOURCE_DIR}/include)

set_property(TARGET sgct PROPERTY CXX_STANDARD 17)
set_property(TARGET sgct PROPERTY CXX_STANDARD_REQUIRED ON)

install(TARGETS sgct
  RUNTIME DESTINATION bin/release CONFIGURATIONS Release
  ARCHIVE DESTINATION lib/release CONFIGURATIONS Release
  LIBRARY DESTINATION lib/release CONFIGURATIONS Release
  BUNDLE  DESTINATION lib/release CONFIGURATIONS Release
)
  
install(TARGETS sgct
  RUNTIME DESTINATION bin/debug CONFIGURATIONS Debug
  ARCHIVE DESTINATION lib/debug CONFIGURATIONS Debug
  LIBRARY DESTINATION lib/debug CONFIGURATIONS Debug
  BUNDLE  DESTINATION lib/debug CONFIGURATIONS Debug
)
