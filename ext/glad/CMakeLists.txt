##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

set(FILES "src/glad.c")
if (WIN32)
  set(FILES ${FILES} "src/glad_wgl.c")
endif ()
add_library(glad ${FILES})
target_include_directories(glad PUBLIC include)
