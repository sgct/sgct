##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

function (copy_files target)
  # Add the copy command
  foreach(file_i ${ARGN})
    add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${file_i}" $<TARGET_FILE_DIR:${target}>
  )
  endforeach ()
endfunction ()
