##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

set(SGCT_EXAMPLE_SHARED_RESOURCES "${PROJECT_SOURCE_DIR}/src/apps/SharedResources")

add_subdirectory(calibrator)
add_subdirectory(clustertest)
add_subdirectory(datatransfer)
add_subdirectory(domeimageviewer)
add_subdirectory(example1)
if (SGCT_EXAMPLE_FFMPEG)
  add_subdirectory(ffmpegcaptureanddomeimageviewer)
  add_subdirectory(ffmpegcaptureexample)
endif ()
if (SGCT_EXAMPLE_FMOD)
  add_subdirectory(fmod)
endif ()
add_subdirectory(gamepad)
add_subdirectory(heightmapping)
add_subdirectory(multiplerendertargets)
if (SGCT_EXAMPLES_NDI)
  add_subdirectory(heightmappingndisender)
endif ()
add_subdirectory(network)
add_subdirectory(omnistereo)
if (SGCT_EXAMPLES_OPENVR)
  add_subdirectory(openvr)
  add_subdirectory(openvrfulldomeviewer)
endif ()
add_subdirectory(remote)
add_subdirectory(simplenavigation)
if (SGCT_EXAMPLES_OPENAL)
  add_subdirectory(sound)
endif ()
if (SGCT_SPOUT_SUPPORT)
  add_subdirectory(spout)
  add_subdirectory(spoutscreenshot)
  add_subdirectory(spoutsender)
endif ()
add_subdirectory(stitcher)
add_subdirectory(test-runner)
if (SGCT_EXAMPLES_WEBSOCKETS)
  add_subdirectory(websockets)
endif ()
