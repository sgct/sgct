##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(fmodexample main.cpp fmod_audio.cpp fmod_audio.h simple.frag simple.vert)
set_compile_options(fmodexample)
target_link_libraries(fmodexample PRIVATE sgct)

find_package(FMOD REQUIRED)
target_include_directories(fmodexample PRIVATE ${FMOD_INCLUDE_DIR})
target_link_libraries(FMOD_example_opengl PRIVATE ${FMOD_LIBRARY})

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/drumloop.wav"
  "${CMAKE_CURRENT_SOURCE_DIR}/jaguar.wav"
)
copy_files(fmodexample ${files})
copy_sgct_dynamic_libraries(fmodexample)
set_target_properties(fmodexample PROPERTIES FOLDER "Examples")
