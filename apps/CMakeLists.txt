##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

set(SGCT_EXAMPLE_SHARED_RESOURCES "${PROJECT_SOURCE_DIR}/apps/SharedResources")

option(SGCT_EXAMPLES_FFMPEG "Build FFMPEG examples" OFF)
option(SGCT_EXAMPLES_OPENAL "Build OpenAL examples" OFF)
option(SGCT_EXAMPLES_OPENVR "Build OpenVR examples" OFF)
option(SGCT_EXAMPLES_STITCHER "Build Stitcher examples" OFF)

add_subdirectory(calibrator)
add_subdirectory(datatransfer)
add_subdirectory(domeimageviewer)
add_subdirectory(example1)
if (SGCT_EXAMPLES_FFMPEG)
  add_subdirectory(ffmpegcaptureanddomeimageviewer)
  add_subdirectory(ffmpegcapture)
endif ()
add_subdirectory(gamepad)
add_subdirectory(heightmapping)
add_subdirectory(multiplerendertargets)
add_subdirectory(network)
add_subdirectory(omnistereo)
add_subdirectory(simplenavigation)
if (SGCT_EXAMPLES_OPENAL)
  add_subdirectory(sound)
endif ()
if (SGCT_SPOUT_SUPPORT)
  add_subdirectory(spout)
  add_subdirectory(spoutbrowser)
  add_subdirectory(spoutscreenshot)
  add_subdirectory(spoutsender)
endif ()
if (SGCT_EXAMPLES_STITCHER)
  add_subdirectory(stitcher)
endif ()
