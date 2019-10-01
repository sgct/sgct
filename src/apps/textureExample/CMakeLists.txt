# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(textureExample main.cpp simple.frag simple.vert)
set_compile_options(textureExample)
target_link_libraries(textureExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(textureExample ${files})
copy_sgct_dynamic_libraries(textureExample)
set_target_properties(textureExample PROPERTIES FOLDER "Examples")
