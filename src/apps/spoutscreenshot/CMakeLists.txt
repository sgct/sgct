# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(spoutscreenshot main.cpp)
set_compile_options(spoutscreenshot)
target_link_libraries(spoutscreenshot PRIVATE sgct)
copy_sgct_dynamic_libraries(spoutscreenshot)
set_target_properties(spoutscreenshot PROPERTIES FOLDER "Examples")
