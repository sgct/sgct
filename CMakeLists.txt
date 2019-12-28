cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(sgct)

include(support/cmake/copy_files.cmake)
include(support/cmake/copy_sgct_dynamic_libraries.cmake)
include(support/cmake/set_compile_options.cmake)
include(support/cmake/disable_external_warnings.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/support/cmake/modules)

set(SGCT_VERSION_MAJOR "3")
set(SGCT_VERSION_MINOR "0")
set(SGCT_VERSION_REVISION "0")
set(SGCT_VERSION "${SGCT_VERSION_MAJOR}_${SGCT_VERSION_MINOR}_${SGCT_VERSION_REVISION}")

if (APPLE)
  set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "OSX Architectures" FORCE)
endif ()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(SGCT_EXAMPLES "Build SGCT examples" OFF)
option(SGCT_FREETYPE_SUPPORT "Build SGCT with Freetype2" ON)
option(SGCT_OPENVR_SUPPORT "SGCT OpenVR support" OFF)

if (WIN32)
  option(SGCT_SPOUT_SUPPORT "SGCT Spout support" OFF)
endif ()

# Exceptions for external libraries
option(SGCT_DEP_INCLUDE_GLFW "Include GLFW library" ON)
option(SGCT_DEP_INCLUDE_LIBPNG "Include LibPNG library" ON)
option(SGCT_DEP_INCLUDE_FREETYPE "Include FreeType library" ON)
option(SGCT_DEP_INCLUDE_TINYXML "Include TinyXML library" ON)
option(SGCT_DEP_ENABLE_TRACY "Enable Tracy Profiler" OFF)
option(SGCT_DEP_INCLUDE_VRPN "Include VRPN library" ON)
option(SGCT_DEP_INCLUDE_GLM "Include GLM library" ON)
option(SGCT_DEP_INCLUDE_GLBINDING "Include glbinding library" ON)

add_subdirectory(src/sgct)
add_subdirectory(ext)

if (SGCT_EXAMPLES)
  option(SGCT_EXAMPLE_FFMPEG "Build FFMPEG examples" OFF)
  option(SGCT_EXAMPLE_FMOD "Build FMOD examples" OFF)
  option(SGCT_EXAMPLE_IMGUI "Build ImGUI examples" OFF)
  option(SGCT_EXAMPLES_NDI "Build NDI examples" OFF)
  option(SGCT_EXAMPLES_OPENAL "Build OpenAL examples" OFF)
  option(SGCT_EXAMPLES_OPENVR "Build OpenVR examples" OFF)
  option(SGCT_EXAMPLES_RGBEASY "Build RGBEasy examples" OFF)
  option(SGCT_EXAMPLES_WEBSOCKETS "Build WebSocket examples" OFF)
  add_subdirectory(src/apps)
endif ()
