# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(FFmpegCaptureExample main.cpp capture.h capture.cpp xform.frag xform.vert)
set_compile_options(FFmpegCaptureExample)
target_link_libraries(FFmpegCaptureExample PRIVATE sgct)

find_package(FFmpeg REQUIRED)
target_include_directories(FFmpegCaptureExample PRIVATE
  ${FFMPEG_ROOT}/include
  ${FFMPEG_LIBAVCODEC_INCLUDE_DIRS}
  ${FFMPEG_LIBAVUTIL_INCLUDE_DIRS}
  ${FFMPEG_LIBAVDEVICE_INCLUDE_DIRS}
  ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS}
  ${FFMPEG_LIBSWSCALE_INCLUDE_DIRS}
  ${FFMPEG_LIBAVRESAMPLE_INCLUDE_DIRS}
)
target_link_libraries(FFmpegCaptureExample PRIVATE
  ${FFMPEG_LIBAVCODEC_LIBRARIES}
  ${FFMPEG_LIBAVUTIL_LIBRARIES}
  ${FFMPEG_LIBAVDEVICE_LIBRARIES}
  ${FFMPEG_LIBAVFORMAT_LIBRARIES}
  ${FFMPEG_LIBSWSCALE_LIBRARIES}
  ${FFMPEG_LIBAVRESAMPLE_LIBRARIES}
)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.vert"
)
copy_files(FFmpegCaptureExample ${files})
copy_sgct_dynamic_libraries(FFmpegCaptureExample)
set_target_properties(FFmpegCaptureExample PROPERTIES FOLDER "Examples")
