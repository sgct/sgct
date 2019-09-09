set(HEADER_FILES
  ${PROJECT_SOURCE_DIR}/include/sgct.h
  ${PROJECT_SOURCE_DIR}/include/sgct/ClusterManager.h
  ${PROJECT_SOURCE_DIR}/include/sgct/Engine.h
  ${PROJECT_SOURCE_DIR}/include/sgct/Font.h
  ${PROJECT_SOURCE_DIR}/include/sgct/FontManager.h
  ${PROJECT_SOURCE_DIR}/include/freetype.h
  ${PROJECT_SOURCE_DIR}/include/Image.h
  ${PROJECT_SOURCE_DIR}/include/MessageHandler.h
  ${PROJECT_SOURCE_DIR}/include/NetworkManager.h
  ${PROJECT_SOURCE_DIR}/include/ReadConfig.h
  ${PROJECT_SOURCE_DIR}/include/SGCTNetwork.h
  ${PROJECT_SOURCE_DIR}/include/SGCTNode.h
  ${PROJECT_SOURCE_DIR}/include/SGCTWindow.h
  ${PROJECT_SOURCE_DIR}/include/Shader.h
  ${PROJECT_SOURCE_DIR}/include/ShaderManager.h
  ${PROJECT_SOURCE_DIR}/include/ShaderProgram.h
  ${PROJECT_SOURCE_DIR}/include/SharedData.h
  ${PROJECT_SOURCE_DIR}/include/Statistics.h
  ${PROJECT_SOURCE_DIR}/include/TextureManager.h
  ${PROJECT_SOURCE_DIR}/include/Viewport.h
)

set(SOURCE_FILES
  ClusterManager.cpp
  Engine.cpp
  Font.cpp
  FontManager.cpp
  freetype.cpp
  Image.cpp
  MessageHandler.cpp
  NetworkManager.cpp
  ReadConfig.cpp
  SGCTNetwork.cpp
  SGCTNode.cpp
  SGCTWindow.cpp
  Shader.cpp
  ShaderManager.cpp
  ShaderProgram.cpp
  SharedData.cpp
  SpoutOutputProjection.cpp
  Statistics.cpp
  TextureManager.cpp
  Viewport.cpp
)

source_group("Source Files" FILES ${SOURCE_FILES})

add_library(sgct ${SOURCE_FILES} ${HEADER_FILES})
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
