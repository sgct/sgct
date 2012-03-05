

project(sgct)

cmake_minimum_required(VERSION 2.6)

# Disable deprecation warnings for standard C functions.
if(CMAKE_COMPILER_2005)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif(CMAKE_COMPILER_2005)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_definitions(-DDARWIN)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

#--------------------------------------------------------------------
# Set preprocessor definition to indicate whether 
# to use the debug postfix
#
if(DEBUG_POSTFIX)
	add_definitions(-D_DEBUG_POSTFIX)
endif(DEBUG_POSTFIX)


###########
# SOURCES #
###########

include_directories(
	${SGCT_INCLUDE_DIRS}
)

link_directories(
	${LIBRARY_OUTPUT_PATH}
)

add_definitions(-DEXAMPLE)

set(SGCT_INCLUDE_DIR ${SGCT_INCLUDE_DIRS}/sgct)

set(HEADER_FILES
	${SGCT_INCLUDE_DIRS}/sgct.h
	${SGCT_INCLUDE_DIR}/ClusterManager.h
	${SGCT_INCLUDE_DIR}/Engine.h
	${SGCT_INCLUDE_DIR}/Font.h
	${SGCT_INCLUDE_DIR}/FontManager.h
	${SGCT_INCLUDE_DIR}/freetype.h
	${SGCT_INCLUDE_DIR}/Image.h
	${SGCT_INCLUDE_DIR}/MessageHandler.h
	${SGCT_INCLUDE_DIR}/NetworkManager.h
	${SGCT_INCLUDE_DIR}/ReadConfig.h
	${SGCT_INCLUDE_DIR}/SGCTNetwork.h
	${SGCT_INCLUDE_DIR}/SGCTNode.h
	${SGCT_INCLUDE_DIR}/SGCTWindow.h
	${SGCT_INCLUDE_DIR}/Shader.h
	${SGCT_INCLUDE_DIR}/ShaderManager.h
	${SGCT_INCLUDE_DIR}/ShaderProgram.h
	${SGCT_INCLUDE_DIR}/SharedData.h
	${SGCT_INCLUDE_DIR}/Statistics.h
	${SGCT_INCLUDE_DIR}/TextureManager.h
	${SGCT_INCLUDE_DIR}/Timer.h
	${SGCT_INCLUDE_DIR}/Viewport.h
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
	Statistics.cpp
	TextureManager.cpp
	Timer.cpp
	Viewport.cpp

)
    
source_group("Source Files" FILES ${SOURCE_FILES})

################
# APPLICATIONS #
################

add_library(sgct 
	${SOURCE_FILES} 
	${HEADER_FILES} 
)


add_dependencies(sgct ${SGCT_DEPENDENCIES})

###########
# LINKING #
###########

TARGET_LINK_LIBRARIES(sgct 	
			${GLFW_LIBRARY} 
			${EXTRA_LIBS}
        	${GLEW_LIBRARY}
			${OpenGL_LIBRARIES}
			${EXTRA_LIBS}
			${PNG_LIBRARY}
			${TINYXML_LIBRARIES}
			${FREETYPE_LIBRARIES}
			)

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

