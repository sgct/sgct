##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

function (copy_sgct_dynamic_libraries target)
  set(libs "")
  get_target_property(libs sgct DYNAMIC_LIBS)
  if (libs)
    # libs will evaluate to false if no properties have been set
    foreach (i ${libs})
      copy_files(${target} ${i})
    endforeach ()
  endif ()
endfunction()
