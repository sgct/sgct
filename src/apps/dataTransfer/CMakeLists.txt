# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(dataTransfer main.cpp xform.frag xform.vert)
set_compile_options(dataTransfer)
target_link_libraries(dataTransfer PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(dataTransfer ${files})
copy_sgct_dynamic_libraries(dataTransfer)
set_target_properties(dataTransfer PROPERTIES FOLDER "Examples")
