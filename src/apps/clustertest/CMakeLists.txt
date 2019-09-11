# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(clustertest main.cpp)
set_compile_options(clustertest)
target_link_libraries(clustertest PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
)
copy_files(clustertest ${files})
copy_sgct_dynamic_libraries(clustertest)
