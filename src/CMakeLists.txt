##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

include(${PROJECT_SOURCE_DIR}/support/cmake/set_compile_options.cmake)
include(GenerateExportHeader)

add_library(sgct)
add_library(sgct::sgct ALIAS sgct)
set_compile_options(sgct)
# set_compile_options links the interface-only compile_settings target PRIVATE, which would
# otherwise leave a dangling $<LINK_ONLY:compile_settings> in the exported interface
set_property(TARGET sgct PROPERTY LINK_LIBRARIES "$<BUILD_INTERFACE:compile_settings>")
set_property(TARGET sgct PROPERTY INTERFACE_LINK_LIBRARIES "")
# compile_settings also carried the language-standard requirement; keep that part in the
# exported interface so that a find_package(sgct) consumer compiles the public headers
# (defaulted comparison operators, std::format, ...) with the right standard
target_compile_features(sgct PUBLIC cxx_std_23)
generate_export_header(sgct
  EXPORT_FILE_NAME include/sgct/sgctexports.h
)

configure_file(
  ${PROJECT_SOURCE_DIR}/src/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/sgct/version.h @ONLY
)

target_include_directories(sgct
  PUBLIC $<INSTALL_INTERFACE:include>
)

target_sources(
  sgct
  PUBLIC
  FILE_SET HEADERS
  TYPE HEADERS
  BASE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${PROJECT_SOURCE_DIR}/include
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/include/sgct/sgctexports.h
    ${CMAKE_CURRENT_BINARY_DIR}/include/sgct/version.h
    ${PROJECT_SOURCE_DIR}/include/sgct/actions.h
    ${PROJECT_SOURCE_DIR}/include/sgct/baseviewport.h
    ${PROJECT_SOURCE_DIR}/include/sgct/callbackdata.h
    ${PROJECT_SOURCE_DIR}/include/sgct/clustermanager.h
    ${PROJECT_SOURCE_DIR}/include/sgct/commandline.h
    ${PROJECT_SOURCE_DIR}/include/sgct/config.h
    ${PROJECT_SOURCE_DIR}/include/sgct/correctionmesh.h
    ${PROJECT_SOURCE_DIR}/include/sgct/definitions.h
    ${PROJECT_SOURCE_DIR}/include/sgct/engine.h
    ${PROJECT_SOURCE_DIR}/include/sgct/error.h
    ${PROJECT_SOURCE_DIR}/include/sgct/format.h
    ${PROJECT_SOURCE_DIR}/include/sgct/font.h
    ${PROJECT_SOURCE_DIR}/include/sgct/fontmanager.h
    ${PROJECT_SOURCE_DIR}/include/sgct/freetype.h
    ${PROJECT_SOURCE_DIR}/include/sgct/image.h
    ${PROJECT_SOURCE_DIR}/include/sgct/internalshaders.h
    ${PROJECT_SOURCE_DIR}/include/sgct/joystick.h
    ${PROJECT_SOURCE_DIR}/include/sgct/keys.h
    ${PROJECT_SOURCE_DIR}/include/sgct/log.h
    ${PROJECT_SOURCE_DIR}/include/sgct/math.h
    ${PROJECT_SOURCE_DIR}/include/sgct/modifiers.h
    ${PROJECT_SOURCE_DIR}/include/sgct/mouse.h
    ${PROJECT_SOURCE_DIR}/include/sgct/mutexes.h
    ${PROJECT_SOURCE_DIR}/include/sgct/network.h
    ${PROJECT_SOURCE_DIR}/include/sgct/networkmanager.h
    ${PROJECT_SOURCE_DIR}/include/sgct/node.h
    ${PROJECT_SOURCE_DIR}/include/sgct/offscreenbuffer.h
    ${PROJECT_SOURCE_DIR}/include/sgct/opengl.h
    ${PROJECT_SOURCE_DIR}/include/sgct/profiling.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection.h
    ${PROJECT_SOURCE_DIR}/include/sgct/screencapture.h
    ${PROJECT_SOURCE_DIR}/include/sgct/sgct.h
    ${PROJECT_SOURCE_DIR}/include/sgct/shadermanager.h
    ${PROJECT_SOURCE_DIR}/include/sgct/shaderprogram.h
    ${PROJECT_SOURCE_DIR}/include/sgct/shareddata.h
    ${PROJECT_SOURCE_DIR}/include/sgct/statisticsrenderer.h
    ${PROJECT_SOURCE_DIR}/include/sgct/texturemanager.h
    ${PROJECT_SOURCE_DIR}/include/sgct/tinyxml.h
    ${PROJECT_SOURCE_DIR}/include/sgct/user.h
    ${PROJECT_SOURCE_DIR}/include/sgct/viewport.h
    ${PROJECT_SOURCE_DIR}/include/sgct/window.h
    ${PROJECT_SOURCE_DIR}/include/sgct/correction/buffer.h
    ${PROJECT_SOURCE_DIR}/include/sgct/correction/domeprojection.h
    ${PROJECT_SOURCE_DIR}/include/sgct/correction/obj.h
    ${PROJECT_SOURCE_DIR}/include/sgct/correction/paulbourke.h
    ${PROJECT_SOURCE_DIR}/include/sgct/correction/pfm.h
    ${PROJECT_SOURCE_DIR}/include/sgct/correction/scalable.h
    ${PROJECT_SOURCE_DIR}/include/sgct/correction/sciss.h
    ${PROJECT_SOURCE_DIR}/include/sgct/correction/simcad.h
    ${PROJECT_SOURCE_DIR}/include/sgct/correction/skyskan.h
    ${PROJECT_SOURCE_DIR}/include/sgct/openxr.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/cubemap.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/cylindrical.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/equirectangular.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/fisheye.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/nonlinearprojection.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/projectionplane.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/sphericalmirror.h

  PRIVATE
    baseviewport.cpp
    clustermanager.cpp
    commandline.cpp
    config.cpp
    correctionmesh.cpp
    engine.cpp
    error.cpp
    font.cpp
    fontmanager.cpp
    freetype.cpp
    image.cpp
    log.cpp
    math.cpp
    network.cpp
    networkmanager.cpp
    node.cpp
    offscreenbuffer.cpp
    openxr.cpp
    profiling.cpp
    projection.cpp
    screencapture.cpp
    shadermanager.cpp
    shaderprogram.cpp
    shareddata.cpp
    statisticsrenderer.cpp
    texturemanager.cpp
    user.cpp
    viewport.cpp
    window.cpp
    correction/domeprojection.cpp
    correction/obj.cpp
    correction/paulbourke.cpp
    correction/pfm.cpp
    correction/scalable.cpp
    correction/sciss.cpp
    correction/simcad.cpp
    correction/skyskan.cpp
    projection/cubemap.cpp
    projection/cylindrical.cpp
    projection/equirectangular.cpp
    projection/fisheye.cpp
    projection/nonlinearprojection.cpp
    projection/projectionplane.cpp
    projection/sphericalmirror.cpp
)

target_precompile_headers(sgct PRIVATE
  <sgct/config.h>
  <sgct/engine.h>
  <sgct/format.h>
  <sgct/opengl.h>
  <glm/glm.hpp>

  <scn/scan.h>

  <algorithm>
  <array>
  <chrono>
  <filesystem>
  <functional>
  <map>
  <memory>
  <string>
  <string_view>
  <utility>
  <variant>
  <vector>
)

find_package(Freetype REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(minizip CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenXR CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(scn CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

# stb is header-only and ships no imported target, so wrap it in one
if (NOT TARGET sgct_stb)
  add_library(sgct_stb INTERFACE IMPORTED)
  add_library(sgct::stb ALIAS sgct_stb)
  target_include_directories(sgct_stb SYSTEM INTERFACE ${Stb_INCLUDE_DIR})
endif ()

if (WIN32)
  find_package(Spout2 CONFIG REQUIRED)

  # The vulkan port pulls in vulkan-headers and vulkan-loader, so Vulkan is always present
  # on Windows and the OpenXR Vulkan fallback is always compiled in
  find_package(Vulkan REQUIRED)
  target_compile_definitions(sgct PRIVATE SGCT_HAS_OPENXR_VULKAN_FALLBACK)
  set_property(TARGET Vulkan::Vulkan PROPERTY IMPORTED_LOCATION "${Vulkan_LIBRARY}")
  set_property(TARGET Vulkan::Vulkan PROPERTY IMPORTED_LOCATION_RELEASE "${Vulkan_LIBRARY}")
endif ()

if (SGCT_TRACY_SUPPORT)
  find_package(Threads REQUIRED) # Needed by Tracy
  find_package(Tracy CONFIG REQUIRED)
endif ()

target_link_libraries(sgct
  PUBLIC
    Freetype::Freetype
    glad::glad
    glfw
    OpenGL::GL
    OpenXR::openxr_loader
    $<$<BOOL:${WIN32}>:Spout2::SpoutLibrary>
    $<$<BOOL:${SGCT_TRACY_SUPPORT}>:Tracy::TracyClient>
  PRIVATE
    ZLIB::ZLIB
    # Spelling prescribed by the minizip port's own usage file: the target name differs
    # between the shared and static builds of that port
    $<IF:$<TARGET_EXISTS:MINIZIP::minizip>,MINIZIP::minizip,MINIZIP::minizipstatic>
    glm::glm
    PNG::PNG
    scn::scn
    sgct::stb
    tinyxml2::tinyxml2
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator::validator
    $<$<BOOL:${SGCT_SCALABLE_SUPPORT}>:scalable>
    $<$<BOOL:${SGCT_NDI_SUPPORT}>:ndi>
    $<$<BOOL:${WIN32}>:Vulkan::Vulkan>
)

target_compile_definitions(sgct
  PUBLIC
    SGCT_HAS_TEXT
    SGCT_HAS_OPENXR
    $<$<BOOL:${WIN32}>:SGCT_HAS_SPOUT>
    $<$<BOOL:${SGCT_MEMORY_PROFILING}>:SGCT_OVERRIDE_NEW_AND_DELETE>
  PRIVATE
    $<$<BOOL:${SGCT_SCALABLE_SUPPORT}>:SGCT_HAS_SCALABLE>
    $<$<BOOL:${SGCT_NDI_SUPPORT}>:SGCT_HAS_NDI>
    $<$<BOOL:${WIN32}>:_CRT_SECURE_NO_WARNINGS>
)

if (WIN32)
  target_link_libraries(sgct PRIVATE ws2_32)
else () # Linux
  find_package(X11 REQUIRED)
  find_package(Threads REQUIRED)
  target_link_libraries(sgct
    PRIVATE
      X11::X11 X11::Xrandr X11::Xinerama X11::Xi X11::Xxf86vm X11::Xcursor
  )
endif ()

if (PROJECT_IS_TOP_LEVEL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  # The exported sgct target refers to these proprietary SDKs by target name, so their
  # binaries have to ship along and sgctConfig.cmake has to recreate the imported targets
  foreach (sdk IN ITEMS scalable ndi)
    if (TARGET ${sdk})
      string(TOUPPER ${sdk} sdk_var)
      get_target_property(sdk_implib ${sdk} IMPORTED_IMPLIB)
      get_target_property(sdk_library ${sdk} IMPORTED_LOCATION)
      cmake_path(GET sdk_implib FILENAME SGCT_${sdk_var}_IMPLIB)
      cmake_path(GET sdk_library FILENAME SGCT_${sdk_var}_LIBRARY)
      install(FILES ${sdk_implib} DESTINATION ${CMAKE_INSTALL_LIBDIR})
      install(FILES ${sdk_library} DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif ()
  endforeach ()

  install(TARGETS sgct EXPORT sgctTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  install(
    EXPORT sgctTargets
    FILE sgctTargets.cmake
    NAMESPACE sgct::
    DESTINATION ${CMAKE_INSTALL_DATADIR}/sgct
  )
  configure_file(
    ${PROJECT_SOURCE_DIR}/support/cmake/sgctConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/sgctConfig.cmake
    @ONLY
  )
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/sgctConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/sgctConfig.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/sgctConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_DATADIR}/sgct
  )
endif ()
