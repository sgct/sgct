##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(omnistereo main.cpp)
set_compile_options(omnistereo)
target_link_libraries(omnistereo PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/sepmap.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/test.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/turnmap.jpg"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(omnistereo ${files})
copy_sgct_dynamic_libraries(omnistereo)
set_target_properties(omnistereo PROPERTIES FOLDER "Examples")
