##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(openvrfulldomeviewer main.cpp xform.frag xform.vert)
set_compile_options(openvrfulldomeviewer)
target_link_libraries(openvrfulldomeviewer PRIVATE sgct)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/support/cmake/modules/")

# OpenVR Specific
find_package(OpenVR REQUIRED)
target_include_directories(openvrfulldomeviewer PRIVATE
  ${OPENVR_INCLUDE_DIRS}
)
target_link_libraries(openvrfulldomeviewer PRIVATE
  ${OPENVR_LIBRARY}
)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye_left.jpg"
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye_right.jpg"
  "${CMAKE_CURRENT_SOURCE_DIR}/openvr_mono.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/openvr_stereo.xml"
)
copy_files(openvrfulldomeviewer ${files})
copy_sgct_dynamic_libraries(openvrfulldomeviewer)
set_property(TARGET openvrfulldomeviewer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:openvrfulldomeviewer>)
set_target_properties(openvrfulldomeviewer PROPERTIES FOLDER "Examples")
