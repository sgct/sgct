# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(SpoutSenderExample main.cpp xform.frag xform.vert)
set_compile_options(SpoutSenderExample)
target_link_libraries(SpoutSenderExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.vert"
  "${SGCT_EXAMPLE_SHARED_RESOURCES}/box.png"
)
copy_files(SpoutSenderExample ${files})
copy_sgct_dynamic_libraries(SpoutSenderExample)
set_target_properties(SpoutSenderExample PROPERTIES FOLDER "Examples")
