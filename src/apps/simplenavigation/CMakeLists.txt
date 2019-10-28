# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(simplenavigation main.cpp)
set_compile_options(simplenavigation)
target_link_libraries(simplenavigation PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/single_fisheye.xml"
)
copy_files(simplenavigation ${files})
copy_sgct_dynamic_libraries(simplenavigation)
set_target_properties(simplenavigation PROPERTIES FOLDER "Examples")
