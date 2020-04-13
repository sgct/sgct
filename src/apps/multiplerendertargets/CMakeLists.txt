##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(multiplerendertargets main.cpp)
set_compile_options(multiplerendertargets)
target_link_libraries(multiplerendertargets PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(multiplerendertargets ${files})
copy_sgct_dynamic_libraries(multiplerendertargets)
set_property(TARGET multiplerendertargets PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:multiplerendertargets>)
set_target_properties(multiplerendertargets PROPERTIES FOLDER "Examples")

set(GLM_ROOT_DIR "${PROJECT_SOURCE_DIR}/ext/glm")
find_package(GLM REQUIRED)
mark_as_advanced(GLM_INCLUDE_DIR GLM_INCLUDE_DIRS)
target_include_directories(multiplerendertargets SYSTEM PRIVATE ${GLM_INCLUDE_DIRS})