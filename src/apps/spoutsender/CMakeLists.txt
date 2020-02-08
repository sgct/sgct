##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(spoutsender main.cpp)
set_compile_options(spoutsender)
target_link_libraries(spoutsender PRIVATE sgct)

set(files "${SGCT_EXAMPLE_SHARED_RESOURCES}/box.png")
copy_files(spoutsender ${files})
copy_sgct_dynamic_libraries(spoutsender)
set_target_properties(spoutsender PROPERTIES FOLDER "Examples")

set(GLM_ROOT_DIR "${PROJECT_SOURCE_DIR}/ext/glm")
find_package(GLM REQUIRED)
mark_as_advanced(GLM_INCLUDE_DIR GLM_INCLUDE_DIRS)
target_include_directories(spoutsender SYSTEM PRIVATE ${GLM_INCLUDE_DIRS})