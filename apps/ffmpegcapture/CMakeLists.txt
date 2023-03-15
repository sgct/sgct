##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(ffmpegcapture main.cpp capture.h capture.cpp)
set_compile_options(ffmpegcapture)
target_link_libraries(ffmpegcapture PRIVATE sgct::sgct)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/user_cmake/Modules)

find_package(FFmpeg REQUIRED)
target_include_directories(ffmpegcapture PRIVATE
  ${FFMPEG_ROOT_DIR}/include
  ${FFMPEG_LIBAVCODEC_INCLUDE_DIRS}
  ${FFMPEG_LIBAVUTIL_INCLUDE_DIRS}
  ${FFMPEG_LIBAVDEVICE_INCLUDE_DIRS}
  ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS}
  ${FFMPEG_LIBSWSCALE_INCLUDE_DIRS}
  ${FFMPEG_LIBAVRESAMPLE_INCLUDE_DIRS}
)
target_link_libraries(ffmpegcapture PRIVATE
  ${FFMPEG_LIBAVCODEC_LIBRARIES}
  ${FFMPEG_LIBAVUTIL_LIBRARIES}
  ${FFMPEG_LIBAVDEVICE_LIBRARIES}
  ${FFMPEG_LIBAVFORMAT_LIBRARIES}
  ${FFMPEG_LIBSWSCALE_LIBRARIES}
  ${FFMPEG_LIBAVRESAMPLE_LIBRARIES}
)

add_custom_command(
  TARGET ffmpegcapture POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"

  $<TARGET_FILE_DIR:ffmpegcapture>
)
set_property(TARGET ffmpegcapture PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:ffmpegcapture>)
set_target_properties(ffmpegcapture PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:ffmpegcapture>)
  add_custom_command(TARGET ffmpegcapture POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:ffmpegcapture> $<TARGET_FILE_DIR:ffmpegcapture>
    COMMAND_EXPAND_LISTS
  )
endif ()
