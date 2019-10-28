# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(spoutsender main.cpp)
set_compile_options(spoutsender)
target_link_libraries(spoutsender PRIVATE sgct)

set(files "${SGCT_EXAMPLE_SHARED_RESOURCES}/box.png")
copy_files(spoutsender ${files})
copy_sgct_dynamic_libraries(spoutsender)
set_target_properties(spoutsender PROPERTIES FOLDER "Examples")
