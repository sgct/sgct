# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(model_loader_opengl3 main.cpp objloader.h objloader.cpp simple.frag simple.vert)
set_compile_options(model_loader_opengl3)
target_link_libraries(model_loader_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.obj")
copy_files(model_loader_opengl3 ${files})
copy_sgct_dynamic_libraries(model_loader_opengl3)
set_target_properties(model_loader_opengl3 PROPERTIES FOLDER "Examples")
