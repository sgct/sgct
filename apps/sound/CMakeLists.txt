##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(sound main.cpp)
set_compile_options(sound)
target_link_libraries(sound PRIVATE sgct::sgct)

find_library(ALUT_DEBUG_LIBRARY NAMES alutd libalutd libalut_cpp11d REQUIRED)
find_library(ALUT_RELEASE_LIBRARY NAMES alut libalut libalut_cpp11 REQUIRED)
set(ALUT_LIBRARY
  debug ${ALUT_DEBUG_LIBRARY}
  optimized ${ALUT_RELEASE_LIBRARY}
)
find_path(ALUT_INCLUDE_DIRECTORY NAMES AL/alut.h REQUIRED)

find_package(OpenAL REQUIRED)

target_include_directories(soundPRIVATE ${OPENAL_INCLUDE_DIRECTORY} ${ALUT_INCLUDE_DIRECTORY})
target_link_libraries(sound PRIVATE ${OPENAL_LIBRARY} ${ALUT_LIBRARY})

add_custom_command(
  TARGET soundExample POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/file1.wav"

  $<TARGET_FILE_DIR:soundExample>
)
set_property(TARGET sound PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:sound>)
set_target_properties(sound PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:sound>)
  add_custom_command(TARGET sound POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:sound> $<TARGET_FILE_DIR:sound>
    COMMAND_EXPAND_LISTS
  )
endif ()
