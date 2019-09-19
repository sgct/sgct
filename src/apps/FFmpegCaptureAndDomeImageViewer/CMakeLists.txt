# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(FFmpegCaptureAndDomeImageViewer main.cpp capture.h capture.cpp)
set_compile_options(FFmpegCaptureAndDomeImageViewer)
target_link_libraries(FFmpegCaptureAndDomeImageViewer PRIVATE sgct)

find_package(FFmpeg REQUIRED)
target_include_directories(FFmpegCaptureAndDomeImageViewer PRIVATE
  ${FFMPEG_ROOT}/include
  ${FFMPEG_LIBAVCODEC_INCLUDE_DIRS}
  ${FFMPEG_LIBAVUTIL_INCLUDE_DIRS}
  ${FFMPEG_LIBAVDEVICE_INCLUDE_DIRS}
  ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS}
  ${FFMPEG_LIBSWSCALE_INCLUDE_DIRS}
  ${FFMPEG_LIBAVRESAMPLE_INCLUDE_DIRS}
)
target_link_libraries(FFmpegCaptureAndDomeImageViewer PRIVATE
  ${FFMPEG_LIBAVCODEC_LIBRARIES}
  ${FFMPEG_LIBAVUTIL_LIBRARIES}
  ${FFMPEG_LIBAVDEVICE_LIBRARIES}
  ${FFMPEG_LIBAVFORMAT_LIBRARIES}
  ${FFMPEG_LIBSWSCALE_LIBRARIES}
  ${FFMPEG_LIBAVRESAMPLE_LIBRARIES}
)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/chromakey.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_two_nodes.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_two_nodes.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.vert"
)
copy_files(FFmpegCaptureAndDomeImageViewer ${files})
copy_sgct_dynamic_libraries(FFmpegCaptureAndDomeImageViewer)
