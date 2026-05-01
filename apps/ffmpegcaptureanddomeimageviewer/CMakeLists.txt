##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(ffmpegcaptureanddomeimageviewer main.cpp capture.h capture.cpp)
set_compile_options(ffmpegcaptureanddomeimageviewer)
target_link_libraries(ffmpegcaptureanddomeimageviewer PRIVATE sgct::sgct)

find_package(FFmpeg REQUIRED)
target_include_directories(ffmpegcaptureanddomeimageviewer PRIVATE
  ${FFMPEG_ROOT_DIR}/include
  ${FFMPEG_LIBAVCODEC_INCLUDE_DIRS}
  ${FFMPEG_LIBAVUTIL_INCLUDE_DIRS}
  ${FFMPEG_LIBAVDEVICE_INCLUDE_DIRS}
  ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS}
  ${FFMPEG_LIBSWSCALE_INCLUDE_DIRS}
  ${FFMPEG_LIBAVRESAMPLE_INCLUDE_DIRS}
)
target_link_libraries(ffmpegcaptureanddomeimageviewer PRIVATE
  ${FFMPEG_LIBAVCODEC_LIBRARIES}
  ${FFMPEG_LIBAVUTIL_LIBRARIES}
  ${FFMPEG_LIBAVDEVICE_LIBRARIES}
  ${FFMPEG_LIBAVFORMAT_LIBRARIES}
  ${FFMPEG_LIBSWSCALE_LIBRARIES}
  ${FFMPEG_LIBAVRESAMPLE_LIBRARIES}
)

add_custom_command(
  TARGET ffmpegcaptureanddomeimageviewer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"

  $<TARGET_FILE_DIR:ffmpegcaptureanddomeimageviewer>
)
set_property(TARGET ffmpegcaptureanddomeimageviewer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:ffmpegcaptureanddomeimageviewer>)
set_target_properties(ffmpegcaptureanddomeimageviewer PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:ffmpegcaptureanddomeimageviewer>)
  add_custom_command(TARGET ffmpegcaptureanddomeimageviewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:ffmpegcaptureanddomeimageviewer> $<TARGET_FILE_DIR:ffmpegcaptureanddomeimageviewer>
    COMMAND_EXPAND_LISTS
  )
endif ()
