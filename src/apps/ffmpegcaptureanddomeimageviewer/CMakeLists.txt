##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2021                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(fmpegcaptureanddomeimageviewer main.cpp capture.h capture.cpp)
set_compile_options(fmpegcaptureanddomeimageviewer)
target_link_libraries(fmpegcaptureanddomeimageviewer PRIVATE sgct)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/user_cmake/Modules)

find_package(FFmpeg REQUIRED)
target_include_directories(fmpegcaptureanddomeimageviewer PRIVATE
  ${FFMPEG_ROOT_DIR}/include
  ${FFMPEG_LIBAVCODEC_INCLUDE_DIRS}
  ${FFMPEG_LIBAVUTIL_INCLUDE_DIRS}
  ${FFMPEG_LIBAVDEVICE_INCLUDE_DIRS}
  ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS}
  ${FFMPEG_LIBSWSCALE_INCLUDE_DIRS}
  ${FFMPEG_LIBAVRESAMPLE_INCLUDE_DIRS}
)
target_link_libraries(fmpegcaptureanddomeimageviewer PRIVATE
  ${FFMPEG_LIBAVCODEC_LIBRARIES}
  ${FFMPEG_LIBAVUTIL_LIBRARIES}
  ${FFMPEG_LIBAVDEVICE_LIBRARIES}
  ${FFMPEG_LIBAVFORMAT_LIBRARIES}
  ${FFMPEG_LIBSWSCALE_LIBRARIES}
  ${FFMPEG_LIBAVRESAMPLE_LIBRARIES}
)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"
)
copy_files(fmpegcaptureanddomeimageviewer ${files})
copy_sgct_dynamic_libraries(fmpegcaptureanddomeimageviewer)
set_property(TARGET fmpegcaptureanddomeimageviewer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:fmpegcaptureanddomeimageviewer>)
set_target_properties(fmpegcaptureanddomeimageviewer PROPERTIES FOLDER "Examples")
