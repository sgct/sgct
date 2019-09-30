# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(SpoutExample main.cpp xform.frag xform.vert)
set_compile_options(SpoutExample)
target_link_libraries(SpoutExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.vert"
)
copy_files(SpoutExample ${files})
copy_sgct_dynamic_libraries(SpoutExample)
set_target_properties(SpoutExample PROPERTIES FOLDER "Examples")
