##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2022                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################
include(CMakePrintHelpers)

set(GSTREAMER_PATH "${PROJECT_SOURCE_DIR}/apps/OpenSpace/ext/sgct/ext/gstreamer")

if (WIN32)
  set(libs
    "${GSTREAMER_PATH}/lib/glib-2.0-0.dll"
    "${GSTREAMER_PATH}/lib/gobject-2.0-0.dll"
    "${GSTREAMER_PATH}/lib/gstapp-1.0-0.dll"
    "${GSTREAMER_PATH}/lib/gstbase-1.0-0.dll"
    "${GSTREAMER_PATH}/lib/gstgl-1.0-0.dll"
    "${GSTREAMER_PATH}/lib/gstpbutils-1.0-0.dll"
    "${GSTREAMER_PATH}/lib/gstreamer-1.0-0.dll"
    "${GSTREAMER_PATH}/lib/gstsdp-1.0-0.dll"
    "${GSTREAMER_PATH}/lib/gstvideo-1.0-0.dll"
    "${GSTREAMER_PATH}/lib/gstwebrtc-1.0-0.dll"
    "${GSTREAMER_PATH}/lib/soup-2.4-1.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/ffi-7.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gio-2.0-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gmodule-2.0-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/graphene-1.0-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstapp.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstaudio-1.0-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstcontroller-1.0-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstcoreelements.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstdtls.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstnet-1.0-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstnice.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstopengl.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstopus.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstrtp.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstrtp-1.0-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstrtpmanager.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstsctp-1.0-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstsrtp.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gsttag-1.0-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstwebrtc.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstx264.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/intl-8.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/libcrypto-1_1-x64.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/libiconv-2.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/libjpeg-8.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/libpng16-16.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/libssl-1_1-x64.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/libx264-157.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/libxml2-2.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/nice-10.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/opus-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/orc-0.4-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/psl-5.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/sqlite3-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/srtp2-1.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/z-1.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstnvcodec.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/bz2.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/harfbuzz.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/libfreetype-6.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstcodecs-1.0-0.dll"
    "${GSTREAMER_PATH}/pluginsanddeps/gstcodecparsers-1.0-0.dll"
  )

  function (copy_gstreamer_dlls target)
      add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${libs} $<TARGET_FILE_DIR:${target}>
      )
  endfunction ()

  function (link_gstreamer_libs target)
    target_include_directories(${target} PUBLIC ${GSTREAMER_PATH})
    target_include_directories(${target} PUBLIC ${GSTREAMER_PATH}/include)
    target_include_directories(${target} PUBLIC ${GSTREAMER_PATH}/include/gstreamer-1.0)
    target_include_directories(${target} PUBLIC ${GSTREAMER_PATH}/include/gstreamer-1.0/gst/gl)
    target_include_directories(${target} PUBLIC ${GSTREAMER_PATH}/include/glib-2.0)
    target_include_directories(${target} PUBLIC ${GSTREAMER_PATH}/lib/glib-2.0/include)
    target_include_directories(${target} PUBLIC ${GSTREAMER_PATH}/lib/gstreamer-1.0/include/)
    target_include_directories(${target} PUBLIC ${GSTREAMER_PATH}/lib/gstreamer-1.0/include/gst)
    target_include_directories(${target} PUBLIC ${GSTREAMER_PATH}/lib/gstreamer-1.0/include/gst/gl)
    target_include_directories(${target} PUBLIC ${GSTREAMER_PATH}/include/glib-2.0)
    target_include_directories(${target} PUBLIC ${GSTREAMER_PATH}/include/libsoup-2.4)
    target_include_directories(${target} PUBLIC ${PROJECT_SOURCE_DIR}/apps/OpenSpace/ext/sgct/ext/json/single_include)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/glib-2.0.lib)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/gobject-2.0.lib)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/gstapp-1.0.lib)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/gstbase-1.0.lib)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/gstgl-1.0.lib)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/gstreamer-1.0.lib)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/gstsdp-1.0.lib)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/gstvideo-1.0.lib)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/gstwebrtc-1.0.lib)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/soup-2.4.lib)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/bz2.lib)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/harfbuzz.lib)
    target_link_libraries(${target} PUBLIC ${GSTREAMER_PATH}/lib/z.lib)
  endfunction()
elseif (UNIX)
  function (link_gstreamer_libs target)
    target_include_directories(${target} PUBLIC /usr/include)
    target_include_directories(${target} PUBLIC ${GSTREAMER_PATH})
    target_include_directories(${target} PUBLIC /usr/include/glib-2.0)
    target_include_directories(${target} PUBLIC /usr/lib/x86_64-linux-gnu/glib-2.0/include)
    target_include_directories(${target} PUBLIC /usr/include/gstreamer-1.0)
    target_include_directories(${target} PUBLIC /usr/lib/x86_64-linux-gnu/gstreamer-1.0/include)
    target_include_directories(${target} PUBLIC /usr/include/libsoup-2.4)
    target_include_directories(${target} PUBLIC ${PROJECT_SOURCE_DIR}/apps/OpenSpace/ext/sgct/ext/json/single_include)

    set(LIBS_SHARED
    libglfw.so
    libgstapp-1.0.so
    libgstbase-1.0.so
    libgstgl-1.0.so
    libgstnvcodecc.so
    libgstreamer-1.0.so
    libgstsdp-1.0.so
    libgstvideo-1.0.so
    libgstwebrtc-1.0.so
    libsoup-2.4.so.1
    libbz2.so.1
    libz.so
    libharfbuzz.so
    libglib-2.0.so
    libgobject-2.0.so)
    foreach(LIB_NAME IN LISTS LIBS_SHARED)
      cmake_print_variables(LIB_NAME)
      find_library(LIB_FOUND ${LIB_NAME})
      if(LIB_FOUND)
        target_link_libraries(${target} PRIVATE ${LIB_FOUND})
        message(STATUS "${LIB_NAME}_DIR: ${LIB_FOUND}")
      else()
        message(WARNING "Shared library ${LIB_NAME} not found")
      endif()
      unset(LIB_FOUND CACHE)
    endforeach()
  endfunction()
endif ()
