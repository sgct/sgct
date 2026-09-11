##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

find_package(common-compile-settings CONFIG REQUIRED)

function (set_compile_options target)
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
