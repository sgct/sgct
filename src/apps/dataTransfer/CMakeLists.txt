# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(datatransfer main.cpp)
set_compile_options(datatransfer)
target_link_libraries(datatransfer PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(datatransfer ${files})
copy_sgct_dynamic_libraries(datatransfer)
set_target_properties(datatransfer PROPERTIES FOLDER "Examples")
