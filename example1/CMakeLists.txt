

project(example1)

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


set(SOURCE_FILES
	main.cpp
)
    
source_group("Source Files" FILES ${SOURCE_FILES})

################
# APPLICATIONS #
################

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
add_executable(example1 MACOSX_BUNDLE ${SOURCE_FILES})
else(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
add_executable(example1 WIN32 ${SOURCE_FILES})
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

add_dependencies(example1 sgct ${SGCT_DEPENDENCIES})

###########
# LINKING #
###########


target_link_libraries(example1
	${SGCT_DEPENDENCIES}
			sgct
			${GLFW_LIBRARY} 
			${EXTRA_LIBS}
        		${GLEW_LIBRARY}
			${OpenGL_LIBRARIES}
			${EXTRA_LIBS}
			${PNG_LIBRARY}
			${TINYXML_LIBRARIES}
			${FREETYPE_LIBRARIES}
	)


install(TARGETS example1
  RUNTIME DESTINATION bin/release CONFIGURATIONS Release
  ARCHIVE DESTINATION lib/release CONFIGURATIONS Release
  LIBRARY DESTINATION lib/release CONFIGURATIONS Release
  BUNDLE  DESTINATION lib/release CONFIGURATIONS Release)
  
  install(TARGETS example1
  RUNTIME DESTINATION bin/debug CONFIGURATIONS Debug
  ARCHIVE DESTINATION lib/debug CONFIGURATIONS Debug
  LIBRARY DESTINATION lib/debug CONFIGURATIONS Debug
  BUNDLE  DESTINATION lib/debug CONFIGURATIONS Debug)

