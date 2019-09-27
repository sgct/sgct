# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(SpoutSenderExample_opengl3 main.cpp)
set_compile_options(SpoutSenderExample_opengl3)
target_link_libraries(SpoutSenderExample_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.vert"
  "${SGCT_EXAMPLE_SHARED_RESOURCES}/box.png"
)
copy_files(SpoutSenderExample_opengl3 ${files})
copy_sgct_dynamic_libraries(SpoutSenderExample_opengl3)
set_target_properties(SpoutSenderExample_opengl3 PROPERTIES FOLDER "Examples")
