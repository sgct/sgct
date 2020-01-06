##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(websockets main.cpp quad.cpp quad.h userData.cpp userData.h
  webserver.cpp webserver.h avatar.frag avatar.vert
)
set_compile_options(websockets)
target_link_libraries(websockets PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/avatar.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/dome.jpg"
  "${CMAKE_CURRENT_SOURCE_DIR}/dome.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
)
copy_files(websockets ${files})
copy_sgct_dynamic_libraries(websockets)
set_target_properties(websockets PROPERTIES FOLDER "Examples")
