# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(websocketsExample
  main.cpp
  quad.cpp
  quad.h
  userData.cpp
  userData.h
  webserver.cpp
  webserver.h
)
set_compile_options(websocketsExample)
target_link_libraries(websocketsExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
)
copy_files(websocketsExample ${files})
copy_sgct_dynamic_libraries(websocketsExample)
set_target_properties(websocketsExample PROPERTIES FOLDER "Examples")
