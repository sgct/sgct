# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(OmniStereoTest main.cpp base.frag base.vert grid.frag grid.vert)
set_compile_options(OmniStereoTest)
target_link_libraries(OmniStereoTest PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/base.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/base.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/grid.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/grid.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/sepmap.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/test.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/turnmap.jpg"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(OmniStereoTest ${files})
copy_sgct_dynamic_libraries(OmniStereoTest)
set_target_properties(OmniStereoTest PROPERTIES FOLDER "Examples")
