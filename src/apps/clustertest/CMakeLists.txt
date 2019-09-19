# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(clustertest main.cpp)
set_compile_options(clustertest)
target_link_libraries(clustertest PRIVATE sgct)
copy_sgct_dynamic_libraries(clustertest)
