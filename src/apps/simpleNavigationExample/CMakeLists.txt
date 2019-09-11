# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(simpleNavigationExample main.cpp)
set_compile_options(simpleNavigationExample)
target_link_libraries(simpleNavigationExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.command"
)
copy_files(simpleNavigationExample ${files})
copy_sgct_dynamic_libraries(simpleNavigationExample)
