##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(clustertest main.cpp)
set_compile_options(clustertest)

find_package(glm REQUIRED)
target_link_libraries(clustertest PRIVATE sgct glm)
set_property(TARGET clustertest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:clustertest>)
set_target_properties(clustertest PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:clustertest>)
  add_custom_command(TARGET clustertest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:clustertest> $<TARGET_FILE_DIR:clustertest>
    COMMAND_EXPAND_LISTS
  )
endif ()
