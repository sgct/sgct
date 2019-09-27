# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(orthoTexture main.cpp)
set_compile_options(orthoTexture)
target_link_libraries(orthoTexture PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/grid.png"
)
copy_files(orthoTexture ${files})
copy_sgct_dynamic_libraries(orthoTexture)
set_target_properties(orthoTexture PROPERTIES FOLDER "Examples")
