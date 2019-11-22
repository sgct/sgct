# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(heightmappingndisender main.cpp ndi.h ndi.cpp heightmap.frag heightmap.vert)
set_compile_options(heightmappingndisender)
target_link_libraries(heightmappingndisender PRIVATE sgct)

find_path(NDI_INCLUDE_DIRECTORY NAMES NDI REQUIRED)
find_library(NDI_LIBRARY NAMES Processing.NDI.Lib.x64 REQUIRED)
target_include_directories(heightmappingndisender PRIVATE ${NDI_INCLUDE_DIRECTORY})
target_link_libraries(heightmappingndisender PRIVATE ${NDI_LIBRARY})

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/heightmap.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/heightmap.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.command"
)
copy_files(heightmappingndisender ${files})
copy_sgct_dynamic_libraries(heightmappingndisender)
set_target_properties(heightmappingndisender PROPERTIES FOLDER "Examples")