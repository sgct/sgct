##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2022                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

set(HEADER_FILES
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
  ${PROJECT_SOURCE_DIR}/include/sgct/mpcdi.h
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
  ${PROJECT_SOURCE_DIR}/include/sgct/correction/mpcdimesh.h
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
  ${PROJECT_SOURCE_DIR}/include/sgct/utils/box.h
  ${PROJECT_SOURCE_DIR}/include/sgct/utils/dome.h
  ${PROJECT_SOURCE_DIR}/include/sgct/utils/domegrid.h
  ${PROJECT_SOURCE_DIR}/include/sgct/utils/plane.h
  ${PROJECT_SOURCE_DIR}/include/sgct/utils/sphere.h
)

set(SOURCE_FILES
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
  mpcdi.cpp
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
  correction/mpcdimesh.cpp
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
  utils/box.cpp
  utils/dome.cpp
  utils/domegrid.cpp
  utils/plane.cpp
  utils/sphere.cpp
)

if (SGCT_VRPN_SUPPORT)
  list(APPEND HEADER_FILES ${PROJECT_SOURCE_DIR}/include/sgct/trackingmanager.h)
  list(APPEND SOURCE_FILES trackingmanager.cpp)
endif ()

if (SGCT_OPENVR_SUPPORT)
  list(APPEND HEADER_FILES ${PROJECT_SOURCE_DIR}/include/sgct/openvr.h)
  list(APPEND SOURCE_FILES openvr.cpp)
endif ()

configure_file(${PROJECT_SOURCE_DIR}/src/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/sgct/version.h @ONLY)
list(APPEND ${HEADER_FILES} ${CMAKE_CURRENT_BINARY_DIR}/include/sgct/version.h)

source_group(TREE ${PROJECT_SOURCE_DIR}/include/sgct PREFIX "Header Files" FILES ${HEADER_FILES})
source_group(TREE ${PROJECT_SOURCE_DIR}/src/sgct PREFIX "Source Files" FILES ${SOURCE_FILES})

add_library(sgct STATIC ${SOURCE_FILES} ${HEADER_FILES})
add_library(sgct::sgct ALIAS sgct)
set_compile_options(sgct)

target_precompile_headers(sgct PRIVATE
  <algorithm>
  <array>
  <map>
  <memory>
  <string>
  <utility>
  <vector>
)

target_include_directories(sgct
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/ext/stb
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

find_package(glad REQUIRED)
find_package(ZLIB REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(PNG REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(OpenGL REQUIRED)
if (SGCT_VRPN_SUPPORT)
  find_package(vrpn REQUIRED)
endif ()
if (SGCT_OPENVR_SUPPORT)
  find_package(openvr REQUIRED)
endif ()

target_link_libraries(sgct 
  PUBLIC 
    glad::glad
    glfw
    fmt
    tracy
    ${OPENGL_gl_LIBRARY}
    $<$<BOOL:${SGCT_FREETYPE_SUPPORT}>:freetype>
    $<$<BOOL:${SGCT_OPENVR_SUPPORT}>:openvr>
  PRIVATE
    ZLIB::ZLIB
    minizip::minizip
    glm
    PNG::PNG
    tinyxml2::tinyxml2
    $<$<BOOL:${SGCT_VRPN_SUPPORT}>:vrpn>
    json
)

target_compile_definitions(sgct 
  PUBLIC 
    $<$<BOOL:${SGCT_DEP_ENABLE_TRACY}>:SGCT_HAS_TRACY>
    $<$<BOOL:${SGCT_FREETYPE_SUPPORT}>:SGCT_HAS_TEXT>
    $<$<BOOL:${SGCT_OPENVR_SUPPORT}>:SGCT_HAS_OPENVR>
    $<$<BOOL:${SGCT_SPOUT_SUPPORT}>:SGCT_HAS_SPOUT>
  PRIVATE
    $<$<BOOL:${SGCT_VRPN_SUPPORT}>:SGCT_HAS_VRPN>
    $<$<BOOL:${WIN32}>:_CRT_SECURE_NO_WARNINGS>
)

if (WIN32 AND SGCT_SPOUT_SUPPORT)
  target_include_directories(sgct PUBLIC ${PROJECT_SOURCE_DIR}/ext/spout)
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
