##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2022                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################
include(CMakePrintHelpers)

set(GSTREAMER_PATH "${PROJECT_SOURCE_DIR}/apps/OpenSpace/ext/sgct/ext/gstreamer")

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
