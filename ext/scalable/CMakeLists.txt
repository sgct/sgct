##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

# This is an SGCT-specific library to interface with Scalable and does not represent the
# full capabilities of this great library. The files contained in this folder are only
# provided to build applications that directly depend on SGCT.
# To access the full ScalableSDK, contact sales@scalabledisplay.com

add_library(scalable SHARED IMPORTED GLOBAL)

target_include_directories(scalable SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(scalable PROPERTIES IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/mplEasyBlendSDK.lib)
set_target_properties(scalable PROPERTIES IMPORTED_IMPLIB_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/mplEasyBlendSDK.lib)
set_target_properties(scalable PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/mplEasyBlendSDK.dll)
