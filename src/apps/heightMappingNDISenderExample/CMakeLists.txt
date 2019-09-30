# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(heightMappingNDISenderExample_opengl3 main.cpp ndi.h ndi.cpp heightmap.frag heightmap.vert)
set_compile_options(heightMappingNDISenderExample_opengl3)
target_link_libraries(heightMappingNDISenderExample_opengl3 PRIVATE sgct)

find_path(NDI_INCLUDE_DIRECTORY NAMES NDI REQUIRED)
find_library(NDI_LIBRARY NAMES Processing.NDI.Lib.x64 REQUIRED)
target_include_directories(heightMappingNDISenderExample_opengl3 PRIVATE ${NDI_INCLUDE_DIRECTORY})
target_link_libraries(heightMappingNDISenderExample_opengl3 PRIVATE ${NDI_LIBRARY})

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/heightmap.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/heightmap.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.command"
)
copy_files(heightMappingNDISenderExample_opengl3 ${files})
copy_sgct_dynamic_libraries(heightMappingNDISenderExample_opengl3)
set_target_properties(heightMappingNDISenderExample_opengl3 PROPERTIES FOLDER "Examples")
