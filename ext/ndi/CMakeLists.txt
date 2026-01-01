##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

# The user of the NDI SDK used under the open-source clause of the license by Vizrt NDI AB
# NDI® is a registered trademark of Vizrt NDI AB
# Check https://ndi.video/ for more information about NDI

add_library(ndi SHARED IMPORTED GLOBAL)

target_include_directories(ndi SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(ndi PROPERTIES IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/Processing.NDI.Lib.x64.lib)
set_target_properties(ndi PROPERTIES IMPORTED_IMPLIB_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/lib/Processing.NDI.Lib.x64.lib)
set_target_properties(ndi PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/Processing.NDI.Lib.x64.dll)
