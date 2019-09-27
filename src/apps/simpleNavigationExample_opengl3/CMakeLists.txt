# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(simpleNavigationExample_opengl3 main.cpp gridShader.frag gridShader.vert
    pyramidShader.frag pyramidShader.vert)
set_compile_options(simpleNavigationExample_opengl3)
target_link_libraries(simpleNavigationExample_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/gridShader.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/gridShader.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/pyramidShader.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/pyramidShader.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.command"
)
copy_files(simpleNavigationExample_opengl3 ${files})
copy_sgct_dynamic_libraries(simpleNavigationExample_opengl3)
set_target_properties(simpleNavigationExample_opengl3 PROPERTIES FOLDER "Examples")
