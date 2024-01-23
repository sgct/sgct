##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

include(${PROJECT_SOURCE_DIR}/support/cmake/set_compile_options.cmake)
include(GenerateExportHeader)

add_library(sgct)
add_library(sgct::sgct ALIAS sgct)
set_compile_options(sgct)
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
    ${PROJECT_SOURCE_DIR}/include/sgct/engine.h
    ${PROJECT_SOURCE_DIR}/include/sgct/error.h
    ${PROJECT_SOURCE_DIR}/include/sgct/fmt.h
    ${PROJECT_SOURCE_DIR}/include/sgct/font.h
    ${PROJECT_SOURCE_DIR}/include/sgct/fontmanager.h
    ${PROJECT_SOURCE_DIR}/include/sgct/freetype.h
    ${PROJECT_SOURCE_DIR}/include/sgct/frustum.h
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
    ${PROJECT_SOURCE_DIR}/include/sgct/readconfig.h
    ${PROJECT_SOURCE_DIR}/include/sgct/screencapture.h
    ${PROJECT_SOURCE_DIR}/include/sgct/sgct.h
    ${PROJECT_SOURCE_DIR}/include/sgct/settings.h
    ${PROJECT_SOURCE_DIR}/include/sgct/shadermanager.h
    ${PROJECT_SOURCE_DIR}/include/sgct/shaderprogram.h
    ${PROJECT_SOURCE_DIR}/include/sgct/shareddata.h
    ${PROJECT_SOURCE_DIR}/include/sgct/statisticsrenderer.h
    ${PROJECT_SOURCE_DIR}/include/sgct/texturemanager.h
    ${PROJECT_SOURCE_DIR}/include/sgct/tinyxml.h
    ${PROJECT_SOURCE_DIR}/include/sgct/tracker.h
    ${PROJECT_SOURCE_DIR}/include/sgct/trackingdevice.h
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
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/cylindrical.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/equirectangular.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/fisheye.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/nonlinearprojection.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/projectionplane.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/sphericalmirror.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/spout.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection/spoutflat.h
    ${PROJECT_SOURCE_DIR}/include/sgct/utils/box.h
    ${PROJECT_SOURCE_DIR}/include/sgct/utils/dome.h
    ${PROJECT_SOURCE_DIR}/include/sgct/utils/domegrid.h
    ${PROJECT_SOURCE_DIR}/include/sgct/utils/plane.h
    ${PROJECT_SOURCE_DIR}/include/sgct/utils/sphere.h

    $<$<BOOL:${SGCT_OPENVR_SUPPORT}>:${PROJECT_SOURCE_DIR}/include/sgct/openvr.h>
    $<$<BOOL:${SGCT_VRPN_SUPPORT}>:${PROJECT_SOURCE_DIR}/include/sgct/trackingmanager.h>

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
    profiling.cpp
    projection.cpp
    readconfig.cpp
    screencapture.cpp
    settings.cpp
    shadermanager.cpp
    shaderprogram.cpp
    shareddata.cpp
    statisticsrenderer.cpp
    texturemanager.cpp
    tracker.cpp
    trackingdevice.cpp
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
    projection/cylindrical.cpp
    projection/equirectangular.cpp
    projection/fisheye.cpp
    projection/nonlinearprojection.cpp
    projection/projectionplane.cpp
    projection/sphericalmirror.cpp
    projection/spout.cpp
    projection/spoutflat.cpp
    utils/box.cpp
    utils/dome.cpp
    utils/domegrid.cpp
    utils/plane.cpp
    utils/sphere.cpp

    $<$<BOOL:${SGCT_OPENVR_SUPPORT}>:openvr.cpp>
    $<$<BOOL:${SGCT_VRPN_SUPPORT}>:trackingmanager.cpp>
)

target_precompile_headers(sgct PRIVATE
  <sgct/config.h>
  <sgct/engine.h>
  <sgct/fmt.h>
  <sgct/opengl.h>
  <glm/glm.hpp>

  <scn/scn.h>

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

target_include_directories(sgct PUBLIC "../../ext/json-schema-validator/src")

find_package(fmt REQUIRED)
find_package(glad REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(minizip REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)
find_package(scn REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(ZLIB REQUIRED)

if (SGCT_TRACY_SUPPORT)
  find_package(Threads REQUIRED) # Needed by Tracy
  find_package(Tracy REQUIRED)
endif ()
if (SGCT_FREETYPE_SUPPORT)
  find_package(Freetype REQUIRED)
endif ()
if (SGCT_OPENVR_SUPPORT)
  find_package(openvr REQUIRED)
endif ()
if (SGCT_VRPN_SUPPORT)
  find_package(vrpn REQUIRED)
endif ()

target_link_libraries(sgct
  PUBLIC
    fmt::fmt
    glad::glad
    glfw
    ${OPENGL_gl_LIBRARY}
    $<$<BOOL:${SGCT_FREETYPE_SUPPORT}>:freetype>
    $<$<BOOL:${SGCT_OPENVR_SUPPORT}>:openvr>
    $<$<BOOL:${SGCT_SPOUT_SUPPORT}>:spout>
    $<$<BOOL:${SGCT_TRACY_SUPPORT}>:Tracy::TracyClient>
  PRIVATE
    ZLIB::ZLIB
    minizip::minizip
    glm::glm
    PNG::PNG
    scn::scn
    sgct::stb
    tinyxml2::tinyxml2
    nlohmann_json::nlohmann_json
    $<$<BOOL:${SGCT_VRPN_SUPPORT}>:vrpn>
    nlohmann_json_schema_validator::nlohmann_json_schema_validator
)

target_compile_definitions(sgct
  PUBLIC
    $<$<BOOL:${SGCT_FREETYPE_SUPPORT}>:SGCT_HAS_TEXT>
    $<$<BOOL:${SGCT_OPENVR_SUPPORT}>:SGCT_HAS_OPENVR>
    $<$<BOOL:${SGCT_SPOUT_SUPPORT}>:SGCT_HAS_SPOUT>
    $<$<BOOL:${SGCT_MEMORY_PROFILING}>:SGCT_OVERRIDE_NEW_AND_DELETE>
  PRIVATE
    $<$<BOOL:${SGCT_VRPN_SUPPORT}>:SGCT_HAS_VRPN>
    $<$<BOOL:${WIN32}>:_CRT_SECURE_NO_WARNINGS>
)

if (WIN32)
  target_link_libraries(sgct PRIVATE ws2_32)
elseif (APPLE)
  find_library(COCOA_LIBRARY Cocoa REQUIRED)
  find_library(IOKIT_LIBRARY IOKit REQUIRED)
  find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
  target_link_libraries(sgct PRIVATE ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
else () # Linux
  find_package(X11 REQUIRED)
  find_package(Threads REQUIRED)
  target_link_libraries(sgct PRIVATE
    ${X11_X11_LIB} ${X11_Xrandr_LIB} ${X11_Xinerama_LIB} ${X11_Xinput_LIB}
    ${X11_Xxf86vm_LIB} ${X11_Xcursor_LIB}
  )
endif ()

if (SGCT_INSTALL)
  include(GNUInstallDirs)
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
  install(
      FILES ${PROJECT_SOURCE_DIR}/support/cmake/sgctConfig.cmake
      DESTINATION ${CMAKE_INSTALL_DATADIR}/sgct
  )
endif ()
