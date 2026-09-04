##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

# Everything SGCT depends on comes from vcpkg, except for what is kept here: VRPN has no
# vcpkg port, and the Scalable and NDI SDKs are proprietary.

# stb is header-only and ships no imported target, so wrap it in one
find_package(Stb REQUIRED)
add_library(sgct_stb INTERFACE IMPORTED GLOBAL)
add_library(sgct::stb ALIAS sgct_stb)
target_include_directories(sgct_stb SYSTEM INTERFACE ${Stb_INCLUDE_DIR})

if (SGCT_VRPN_SUPPORT)
  add_subdirectory(vrpn)
  set_target_properties(vrpn PROPERTIES FOLDER "External")
endif ()

if (WIN32 AND SGCT_SCALABLE_SUPPORT)
  add_subdirectory(scalable SYSTEM)
endif ()

if (WIN32 AND SGCT_NDI_SUPPORT)
  add_subdirectory(ndi SYSTEM)
endif ()
