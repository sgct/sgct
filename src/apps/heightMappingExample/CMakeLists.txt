# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(heightMappingExample main.cpp)
set_compile_options(heightMappingExample)
target_link_libraries(heightMappingExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/heightmap.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/normalmap.png"
)
copy_files(heightMappingExample ${files})
copy_sgct_dynamic_libraries(heightMappingExample)
set_target_properties(heightMappingExample PROPERTIES FOLDER "Examples")
