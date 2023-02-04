##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(spoutsender main.cpp)
set_compile_options(spoutsender)
find_package(GLM REQUIRED)
target_link_libraries(spoutsender PRIVATE sgct glm)

set(files "${SGCT_EXAMPLE_SHARED_RESOURCES}/box.png")
copy_files(spoutsender ${files})
set_property(TARGET spoutsender PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:spoutsender>)
set_target_properties(spoutsender PROPERTIES FOLDER "Examples")

if (WIN32)
  add_custom_command(TARGET spoutsender POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:spoutsender> $<TARGET_FILE_DIR:spoutsender>
    COMMAND_EXPAND_LISTS
  )
endif ()
