##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2022                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

set(GSTREAMER_PATH "${PROJECT_SOURCE_DIR}/apps/OpenSpace/ext/sgct/ext/gstreamer")

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