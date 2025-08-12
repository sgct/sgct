##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2025                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

function (set_compile_options target)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/support/cmake/common-compile-settings")
  include(${PROJECT_SOURCE_DIR}/support/cmake/common-compile-settings/common-compile-settings.cmake)
  set_compile_settings(${target})

  if (SGCT_ENABLE_EDIT_CONTINUE)
    # Edit and continue support
    target_compile_options(${target} PRIVATE "/ZI")
  endif ()

  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (SGCT_ENABLE_STATIC_ANALYZER)
      target_compile_options(${target} PRIVATE "-fanalyzer")
    endif ()
  endif ()
endfunction ()
