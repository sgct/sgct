# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(trackingExample main.cpp)
set_compile_options(trackingExample)
target_link_libraries(trackingExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
)
copy_files(trackingExample ${files})

add_executable(trackingExample_test test.cpp)
set_compile_options(trackingExample_test)
target_link_libraries(trackingExample_test PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
)
copy_files(trackingExample_test ${files})
copy_sgct_dynamic_libraries(trackingExample_test)
