# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(simpleNavigationExample main.cpp gridShader.frag gridShader.vert
    pyramidShader.frag pyramidShader.vert)
set_compile_options(simpleNavigationExample)
target_link_libraries(simpleNavigationExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/single_fisheye.xml"
)
copy_files(simpleNavigationExample ${files})
copy_sgct_dynamic_libraries(simpleNavigationExample)
set_target_properties(simpleNavigationExample PROPERTIES FOLDER "Examples")
