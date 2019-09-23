# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(kinectExample main.cpp)
set_compile_options(kinectExample)
target_link_libraries(kinectExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/Kinect.xml"
)
copy_files(kinectExample ${files})
copy_sgct_dynamic_libraries(kinectExample)
