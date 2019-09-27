# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(imguiExample
  main.cpp
)
set_compile_options(imguiExample)
target_link_libraries(imguiExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_single_two_win.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_single_two_win_3D_copy.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/SimpleFragmentShader.fragmentshader"
  "${CMAKE_CURRENT_SOURCE_DIR}/SimpleVertexShader.vertexshader"
  "${CMAKE_CURRENT_SOURCE_DIR}/single_two_win.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/single_two_win_3D_copy.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"
)
copy_files(imguiExample ${files})
copy_sgct_dynamic_libraries(imguiExample)
set_target_properties(imguiExample PROPERTIES FOLDER "Examples")
