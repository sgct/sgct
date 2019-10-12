# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(SpoutScreenshot main.cpp)
set_compile_options(SpoutScreenshot)
target_link_libraries(SpoutScreenshot PRIVATE sgct)
copy_sgct_dynamic_libraries(SpoutScreenshot)
set_target_properties(SpoutScreenshot PROPERTIES FOLDER "Examples")
