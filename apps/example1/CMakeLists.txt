##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(example1 main.cpp)
set_compile_options(example1)
find_package(glm REQUIRED)
target_link_libraries(example1 PRIVATE sgct::sgct glm::glm)
set_property(TARGET example1 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:example1>)
set_target_properties(example1 PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:example1>)
  add_custom_command(TARGET example1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:example1> $<TARGET_FILE_DIR:example1>
    COMMAND_EXPAND_LISTS
  )
endif ()
