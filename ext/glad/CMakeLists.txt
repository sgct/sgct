##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

set(FILES "src/glad.c")
if (WIN32)
  set(FILES ${FILES} "src/glad_wgl.c")
endif ()
add_library(glad STATIC ${FILES})
add_library(glad::glad ALIAS glad)
target_include_directories(glad PUBLIC include)
