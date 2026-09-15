##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

if (WIN32 AND SGCT_SCALABLE_SUPPORT)
  add_subdirectory(scalable SYSTEM)
endif ()

if (WIN32 AND SGCT_NDI_SUPPORT)
  add_subdirectory(ndi SYSTEM)
endif ()
