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
target_compile_features(sgct PUBLIC cxx_std_23)
generate_export_header(sgct EXPORT_FILE_NAME include/sgct/sgctexports.h)

configure_file(
  ${PROJECT_SOURCE_DIR}/src/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/sgct/version.h @ONLY
)

target_include_directories(
  sgct
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
    ${PROJECT_SOURCE_DIR}/include/sgct/profiling.h
    ${PROJECT_SOURCE_DIR}/include/sgct/projection.h
    ${PROJECT_SOURCE_DIR}/include/sgct/screencapture.h
    ${PROJECT_SOURCE_DIR}/include/sgct/sgct.h
    ${PROJECT_SOURCE_DIR}/include/sgct/shareddata.h
    ${PROJECT_SOURCE_DIR}/include/sgct/statisticsrenderer.h
    ${PROJECT_SOURCE_DIR}/include/sgct/tinyxml.h
    ${PROJECT_SOURCE_DIR}/include/sgct/user.h
    ${PROJECT_SOURCE_DIR}/include/sgct/viewport.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan.h
    ${PROJECT_SOURCE_DIR}/include/sgct/window.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/buffer.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/compositepass.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/context.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/cubemaptarget.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/framebuffer.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/offscreenbuffer.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/overlaypass.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/pipeline.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/projectionpass.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/sceneformat.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/swapchain.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/texture.h
    ${PROJECT_SOURCE_DIR}/include/sgct/vulkan/uniformring.h
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
    openxr.cpp
    profiling.cpp
    projection.cpp
    screencapture.cpp
    shareddata.cpp
    statisticsrenderer.cpp
    user.cpp
    viewport.cpp
    window.cpp
    vulkan/buffer.cpp
    vulkan/compositepass.cpp
    vulkan/context.cpp
    vulkan/cubemaptarget.cpp
    vulkan/framebuffer.cpp
    vulkan/imageresource.cpp
    vulkan/imageresource.h
    vulkan/imageresource.inl
    vulkan/offscreenbuffer.cpp
    vulkan/overlaypass.cpp
    vulkan/pipeline.cpp
    vulkan/projectionpass.cpp
    vulkan/sceneformat.cpp
    vulkan/swapchain.cpp
    vulkan/texture.cpp
    vulkan/uniformring.cpp
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
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(minizip CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator CONFIG REQUIRED)
find_package(OpenXR CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(scn CONFIG REQUIRED)
find_package(stbimage CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(ZLIB REQUIRED)

# The Vulkan platform surface extension must be enabled before vulkan.h is first included.
# The precompiled header pulls vulkan.h in through window.h, so this has to be a compile
# definition (active during the PCH build), not a #define in a translation unit
target_compile_definitions(sgct PRIVATE
  $<$<PLATFORM_ID:Windows>:VK_USE_PLATFORM_WIN32_KHR>
  $<$<NOT:$<PLATFORM_ID:Windows>>:VK_USE_PLATFORM_XLIB_KHR>
)

if (WIN32)
  if (SGCT_SPOUT_SUPPORT)
    find_package(Spout2 CONFIG REQUIRED)
  endif ()
endif ()

if (SGCT_TRACY_SUPPORT)
  find_package(Threads REQUIRED) # Needed by Tracy
  find_package(Tracy CONFIG REQUIRED)
endif ()

target_link_libraries(sgct
  PUBLIC
    Freetype::Freetype
    glfw
    OpenXR::openxr_loader
    Vulkan::Vulkan
    $<$<BOOL:${SGCT_SPOUT_SUPPORT}>:Spout2::SpoutLibrary>
    $<$<BOOL:${SGCT_TRACY_SUPPORT}>:Tracy::TracyClient>
    # The static library references the stb_image symbols (image.cpp) but only links
    # the header, so the read implementation must be part of the link interface for
    # consumers to resolve them (the shared library already contains the symbols)
    $<IF:$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>,$<LINK_ONLY:unofficial::stbimage::stbimage-read-impl>,>
  PRIVATE
    ZLIB::ZLIB
    $<IF:$<TARGET_EXISTS:MINIZIP::minizip>,MINIZIP::minizip,MINIZIP::minizipstatic>
    glm::glm
    glslang::glslang
    glslang::glslang-default-resource-limits
    glslang::SPIRV
    PNG::PNG
    scn::scn
    $<IF:$<BOOL:${BUILD_SHARED_LIBS}>,unofficial::stbimage::stbimage-read-impl,unofficial::stbimage::stbimage>
    tinyxml2::tinyxml2
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator::validator
    $<$<BOOL:${SGCT_SCALABLE_SUPPORT}>:scalable>
    $<$<BOOL:${SGCT_NDI_SUPPORT}>:ndi>
)

target_compile_definitions(sgct
  PUBLIC
    SGCT_HAS_TEXT
    SGCT_HAS_OPENXR
    $<$<BOOL:${SGCT_SPOUT_SUPPORT}>:SGCT_HAS_SPOUT>
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
  configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/support/cmake/sgctConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/sgctConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/sgct
    PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_BINDIR
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
