# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(SpoutSenderExample main.cpp)
set_compile_options(SpoutSenderExample)
target_link_libraries(SpoutSenderExample PRIVATE sgct)

set(files
  "${SGCT_EXAMPLE_SHARED_RESOURCES}/box.png"
)
copy_files(SpoutSenderExample ${files})
copy_sgct_dynamic_libraries(SpoutSenderExample)
set_target_properties(SpoutSenderExample PROPERTIES FOLDER "Examples")
