# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(dataTransfer_opengl3 main.cpp xform.frag xform.vert)
set_compile_options(dataTransfer_opengl3)
target_link_libraries(dataTransfer_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(dataTransfer_opengl3 ${files})
copy_sgct_dynamic_libraries(dataTransfer_opengl3)
set_target_properties(dataTransfer_opengl3 PROPERTIES FOLDER "Examples")
