##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2022                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

function(disable_external_warnings target)
  if (MSVC)
    target_compile_options(${target} PRIVATE "/W0")
  else ()
    target_compile_options(${target} PRIVATE "-w")
  endif ()
endfunction ()
