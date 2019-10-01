# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(MRTExample main.cpp mrt.frag mrt.vert)
set_compile_options(MRTExample)
target_link_libraries(MRTExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(MRTExample ${files})
copy_sgct_dynamic_libraries(MRTExample)
set_target_properties(MRTExample PROPERTIES FOLDER "Examples")
