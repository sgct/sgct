
###########
# SOURCES #
###########
link_directories(
	${LIBRARY_OUTPUT_PATH}
)

add_definitions(-DEXAMPLE)

set(SGCT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/)
		message(${SGCT_INCLUDE_DIR})

set(HEADER_FILES
	${SGCT_INCLUDE_DIR}/sgct.h
	${SGCT_INCLUDE_DIR}/sgct/ClusterManager.h
	${SGCT_INCLUDE_DIR}/sgct/Engine.h
	${SGCT_INCLUDE_DIR}/sgct/Font.h
	${SGCT_INCLUDE_DIR}/sgct/FontManager.h
	${SGCT_INCLUDE_DIR}/sgct/freetype.h
	${SGCT_INCLUDE_DIR}/sgct/Image.h
	${SGCT_INCLUDE_DIR}/sgct/MessageHandler.h
	${SGCT_INCLUDE_DIR}/sgct/NetworkManager.h
	${SGCT_INCLUDE_DIR}/sgct/ReadConfig.h
	${SGCT_INCLUDE_DIR}/sgct/SGCTNetwork.h
	${SGCT_INCLUDE_DIR}/sgct/SGCTNode.h
	${SGCT_INCLUDE_DIR}/sgct/SGCTWindow.h
	${SGCT_INCLUDE_DIR}/sgct/Shader.h
	${SGCT_INCLUDE_DIR}/sgct/ShaderManager.h
	${SGCT_INCLUDE_DIR}/sgct/ShaderProgram.h
	${SGCT_INCLUDE_DIR}/sgct/SharedData.h
	${SGCT_INCLUDE_DIR}/sgct/Statistics.h
	${SGCT_INCLUDE_DIR}/sgct/TextureManager.h
	${SGCT_INCLUDE_DIR}/sgct/Viewport.h
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

################
# BUILD #
################

add_library(sgct 
	${SOURCE_FILES} 
	${HEADER_FILES} 
)

add_dependencies(sgct ${SGCT_DEPENDENCIES})

###########
# LINKING #
###########

TARGET_LINK_LIBRARIES(sgct ${SGCT_DEPENDENCIES})

###################
# Install Targets #
###################

install(TARGETS sgct
  RUNTIME DESTINATION bin/release CONFIGURATIONS Release
  ARCHIVE DESTINATION lib/release CONFIGURATIONS Release
  LIBRARY DESTINATION lib/release CONFIGURATIONS Release
  BUNDLE  DESTINATION lib/release CONFIGURATIONS Release)
  
  install(TARGETS sgct
  RUNTIME DESTINATION bin/debug CONFIGURATIONS Debug
  ARCHIVE DESTINATION lib/debug CONFIGURATIONS Debug
  LIBRARY DESTINATION lib/debug CONFIGURATIONS Debug
  BUNDLE  DESTINATION lib/debug CONFIGURATIONS Debug)

