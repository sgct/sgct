# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(multiplerendertargets main.cpp)
set_compile_options(multiplerendertargets)
target_link_libraries(multiplerendertargets PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(multiplerendertargets ${files})
copy_sgct_dynamic_libraries(multiplerendertargets)
set_target_properties(multiplerendertargets PROPERTIES FOLDER "Examples")
