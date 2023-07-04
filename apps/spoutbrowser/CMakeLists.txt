##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(spoutbrowser main.cpp)
set_compile_options(spoutbrowser)
find_package(glm REQUIRED)
target_link_libraries(spoutbrowser PRIVATE sgct::sgct glm::glm)
set_property(TARGET spoutbrowser PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:spoutbrowser>)
set_target_properties(spoutbrowser PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:spoutbrowser>)
  add_custom_command(TARGET spoutbrowser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:spoutbrowser> $<TARGET_FILE_DIR:spoutbrowser>
    COMMAND_EXPAND_LISTS
  )
endif ()
